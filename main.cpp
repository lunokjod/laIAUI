#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "GLFW/glfw3.h"
#include <vector>
#include <iostream>
#include <string>
#include <memory>
#include <atomic>
#include <cstring>
#include <cstdlib>
#include <csignal>
#include <unistd.h>
#include <cstdio>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <future>
#include <fstream>
#include <sstream>

using json = nlohmann::json;
using namespace std;

// Global atomic for interrupt handling
atomic<bool> g_interrupted(false);
atomic<bool> g_inChatLoop(false);

// Token counter class for DeepSeek (compatible with GPT-4 tokenizer)
class TokenCounter {
private:
    // BPE ranks from GPT-4 tokenizer (simplified version)
    static unordered_map<string, int> getBpeRanks() {
        // Aquesta és una versió simplificada. En una implementació real,
        // caldria carregar el vocab.json i merges.txt del tokenizer de DeepSeek
        // Per ara, fem una aproximació basada en caràcters i paraules comunes
        
        unordered_map<string, int> ranks;
        int rank = 0;
        
        // Caràcters individuals (ASCII)
        for (int i = 0; i < 256; i++) {
            string ch(1, static_cast<char>(i));
            ranks[ch] = rank++;
        }
        
        // Patrons comuns en anglès/català
        vector<string> commonPatterns = {
            " the", " a", " an", " of", " to", " in", " for", " on", " with",
            " is", " are", " was", " were", " be", " been", " have", " has",
            " had", " do", " does", " did", " not", " and", " or", " but",
            " if", " then", " else", " when", " where", " why", " how",
            " all", " any", " both", " each", " few", " more", " most",
            " other", " some", " such", " no", " nor", " not", " only",
            " own", " same", " so", " than", " too", " very", " can",
            " will", " just", " don", " should", " now", " el", " la",
            " els", " les", " un", " una", " uns", " unes", " de", " i",
            " que", " en", " és", " no", " per", " amb", " com", " del",
            " al", " als", " de la", " de les", " a la", " a les"
        };
        
        for (const auto& pattern : commonPatterns) {
            ranks[pattern] = rank++;
        }
        
        return ranks;
    }
    
    static vector<pair<string, int>> getByteEncoder() {
        vector<pair<string, int>> encoder;
        
        // Caràcters ASCII
        for (int i = 0; i < 256; i++) {
            encoder.push_back({string(1, static_cast<char>(i)), i});
        }
        
        // Caràcters especials i Unicode (simplificat)
        // En una implementació real, caldria manejar tot Unicode
        
        return encoder;
    }
    
public:
    // Funció per comptar tokens (aproximació per a DeepSeek/GPT-4)
    static int countTokens(const string& text) {
        if (text.empty()) return 0;
        
        // Aproximació basada en:
        // 1 token ≈ 4 caràcters en anglès
        // 1 token ≈ 3-4 caràcters en català/espanyol
        // Per a codi, 1 token ≈ 2-3 caràcters
        
        // Comptar paraules, caràcters i espais per fer una estimació millor
        int charCount = text.length();
        int wordCount = 0;
        int spaceCount = 0;
        int punctuationCount = 0;
        int digitCount = 0;
        
        bool inWord = false;
        for (char c : text) {
            if (isspace(c)) {
                spaceCount++;
                if (inWord) {
                    wordCount++;
                    inWord = false;
                }
            } else if (ispunct(c)) {
                punctuationCount++;
                if (inWord) {
                    wordCount++;
                    inWord = false;
                }
            } else if (isdigit(c)) {
                digitCount++;
                inWord = true;
            } else {
                inWord = true;
            }
        }
        
        // Comptar la última paraula si hi ha
        if (inWord) {
            wordCount++;
        }
        
        // Estimació més precisa basada en l'estadística de DeepSeek:
        // - Paraules comunes: 1 token per paraula
        // - Paraules llargues: múltiples tokens
        // - Punts i espais: generalment 0-1 tokens
        // - Números: 1 token per grup de 2-3 dígits
        
        // Estimació conservadora
        double estimatedTokens = 0;
        
        // Paraules: entre 0.8 i 1.2 tokens per paraula
        estimatedTokens += wordCount * 1.0;
        
        // Punts i espais: afegir una mica
        estimatedTokens += (punctuationCount + spaceCount) * 0.3;
        
        // Números: menys tokens que lletres
        estimatedTokens += digitCount * 0.5;
        
        // Caràcters restants
        int otherChars = charCount - wordCount - punctuationCount - spaceCount - digitCount;
        estimatedTokens += otherChars * 0.25;
        
        // Ajustar per llenguatge: català/espanyol tendeix a tenir més tokens que anglès
        // per la mateixa quantitat de caràcters
        double languageFactor = 1.1; // 10% més per català
        
        // Detectar si hi ha codi (per les cometes inverses)
        bool hasCode = text.find("```") != string::npos || 
                      text.find("#include") != string::npos ||
                      text.find("def ") != string::npos ||
                      text.find("function") != string::npos ||
                      text.find("class ") != string::npos;
        
        if (hasCode) {
            // El codi tendeix a tenir més tokens per caràcter
            languageFactor = 1.3;
        }
        
        estimatedTokens *= languageFactor;
        
        // Assegurar-nos d'un mínim
        if (estimatedTokens < 1 && charCount > 0) {
            estimatedTokens = 1;
        }
        
        return static_cast<int>(estimatedTokens + 0.5); // Arrodonir
    }
    
    // Funció més precisa per a missatges del chat (inclou rols i format)
    static int countChatMessageTokens(const string& role, const string& content) {
        // Els missatges del chat inclouen metadades addicionals
        int totalTokens = 0;
        
        // Tokens per al format del missatge (aproximat)
        // "role": "user", "content": "..." etc.
        totalTokens += 10; // Tokens per a l'estructura JSON
        
        // Tokens pel rol
        totalTokens += countTokens(role) + 2; // +2 per les cometes
        
        // Tokens pel contingut
        totalTokens += countTokens(content);
        
        return totalTokens;
    }
    
    // Funció per estimar tokens en una conversa completa
    static int estimateConversationTokens(const vector<pair<string, string>>& messages) {
        int totalTokens = 0;
        
        // Tokens per a l'estructura del sistema (aproximat)
        totalTokens += 50; // Prompt del sistema, etc.
        
        // Afegir tokens per cada missatge
        for (const auto& [role, content] : messages) {
            totalTokens += countChatMessageTokens(role, content);
        }
        
        // Afegir tokens per a les respostes possibles
        totalTokens += 100; // Reserva per a la resposta
        
        return totalTokens;
    }
};


// Terminal emulator class (simplified for GUI)
class TerminalEmulator {
private:
    string workingDir;
    
public:
    TerminalEmulator(const string& initialDir = "") {
        if (initialDir.empty()) {
            char buffer[1024];
            if (getcwd(buffer, sizeof(buffer))) {
                workingDir = buffer;
            } else {
                workingDir = ".";
            }
        } else {
            workingDir = initialDir;
        }
        
        // Normalitzar el path per assegurar-nos que no hi ha barra final
        if (!workingDir.empty() && workingDir.back() == '/') {
            workingDir.pop_back();
        }
    }
    
    struct CommandResult {
        bool success;
        int returnCode;
        string stdout;
        string stderr;
        string command;
    };

    CommandResult executeCommand(const string& command) {
        char originalDir[1024];
        getcwd(originalDir, sizeof(originalDir));
        
        try {
            if (chdir(workingDir.c_str()) != 0) {
                chdir(originalDir);
                return {false, -1, "", "Error changing directory: " + workingDir, command};
            }
            
            string commandStripped = trim(command);
            
            // Millor solució: Capturar stderr per separat
            // Executar la comanda i capturar stderr en un fitxer temporal
            string tempFile = "/tmp/laia_stderr_" + to_string(time(nullptr)) + ".txt";
            string fullCommand = command + " 2>" + tempFile;
            
            FILE* pipe = popen(fullCommand.c_str(), "r");
            if (!pipe) {
                chdir(originalDir);
                return {false, -1, "", "Error executing command", command};
            }
            
            char buffer[4096];
            string stdoutResult;
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                stdoutResult += buffer;
            }
            
            int returnCode = pclose(pipe);
            bool success = (returnCode == 0);
            
            // Llegir el fitxer temporal amb stderr
            string stderrResult = "";
            FILE* stderrFile = fopen(tempFile.c_str(), "r");
            if (stderrFile) {
                char stderrBuffer[4096];
                while (fgets(stderrBuffer, sizeof(stderrBuffer), stderrFile) != nullptr) {
                    stderrResult += stderrBuffer;
                }
                fclose(stderrFile);
                // Eliminar el fitxer temporal
                remove(tempFile.c_str());
            }
            
            // Update directory if cd command
            if (command.find("cd ") != string::npos && success) {
                char currentDir[1024];
                if (getcwd(currentDir, sizeof(currentDir))) {
                    workingDir = currentDir;
                }
            }
            
            chdir(originalDir);
            return {success, returnCode, stdoutResult, stderrResult, command};
            
        } catch (const exception& e) {
            chdir(originalDir);
            return {false, -1, "", "Error running: " + string(e.what()), command};
        }
    }
    
    string getCurrentDir() const { return workingDir; }
    
    bool setCurrentDir(const string& newDir) {
        try {
            string targetDir = newDir;
            if (targetDir.find("~") == 0) {
                const char* home = getenv("HOME");
                if (home) {
                    targetDir = string(home) + targetDir.substr(1);
                }
            }
            
            if (chdir(targetDir.c_str()) == 0) {
                char buffer[1024];
                if (getcwd(buffer, sizeof(buffer))) {
                    workingDir = buffer;
                    return true;
                }
            }
            return false;
        } catch (const exception& e) {
            return false;
        }
    }
    
private:
    static string trim(const string& str) {
        size_t first = str.find_first_not_of(" \t\n\r");
        if (first == string::npos) return "";
        size_t last = str.find_last_not_of(" \t\n\r");
        return str.substr(first, last - first + 1);
    }
};

// DeepSeek client class (simplified for GUI)
class DeepSeekClient {
private:
    string apiKey;
    string baseUrl;
    string model;
    unique_ptr<TerminalEmulator> terminal;
    vector<json> chatHistory;
    size_t maxChatHistory;
    vector<json> lastCommandResults;
    vector<json> pendingCommandResults;
    function<void(const json&)>* onCommandResult;
    // Estructura per acumular dades de streaming
    struct StreamData {
        string buffer;
        string fullResponse;
        vector<json> toolCalls;
        bool inToolCall;
        function<void(const string&)>* onChunk; // Afegir punter al callback
    };
    int countTextTokens(const string& text) const {
        return TokenCounter::countTokens(text);
    }
    // Callback per streaming
    static size_t StreamingWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        size_t totalSize = size * nmemb;
        char* data = static_cast<char*>(contents);
        
        // userp és un punter a StreamData
        StreamData* streamData = static_cast<StreamData*>(userp);
        
        if (!streamData || !streamData->onChunk) {
            return 0; // Error: dades nul·les
        }
        
        // Afegir les dades al buffer
        streamData->buffer.append(data, totalSize);
        
        // Processar les línies completes immediatament
        size_t newlinePos;
        while ((newlinePos = streamData->buffer.find('\n')) != string::npos) {
            string line = streamData->buffer.substr(0, newlinePos);
            streamData->buffer.erase(0, newlinePos + 1);
            
            if (line.find("data: ") == 0) {
                string dataStr = line.substr(6);
                
                if (dataStr == "[DONE]") {
                    return totalSize;
                }
                
                if (!dataStr.empty()) {
                    try {
                        json chunk = json::parse(dataStr);
                        
                        if (chunk.contains("choices") && !chunk["choices"].empty()) {
                            auto& choice = chunk["choices"][0];
                            
                            if (choice.contains("delta")) {
                                auto& delta = choice["delta"];
                                
                                // Contingut textual
                                if (delta.contains("content") && !delta["content"].is_null()) {
                                    string content = delta["content"];
                                    if (!content.empty()) {
                                        streamData->fullResponse += content;
                                        // Enviar el chunk al callback EN TEMPS REAL
                                        try {
                                            (*streamData->onChunk)(content);
                                        } catch (const bad_function_call& e) {
                                            return 0;
                                        }
                                    }
                                }
                                
                                // Tool calls (chunked)
                                if (delta.contains("tool_calls") && !delta["tool_calls"].empty()) {
                                    for (auto& toolCallDelta : delta["tool_calls"]) {
                                        int index = toolCallDelta.value("index", 0);
                                        
                                        // Assegurar-nos que tenim espai per a aquest tool call
                                        if (index >= streamData->toolCalls.size()) {
                                            streamData->toolCalls.resize(index + 1);
                                        }
                                        
                                        auto& toolCall = streamData->toolCalls[index];
                                        
                                        // Inicialitzar tool call si és necessari
                                        if (toolCall.is_null()) {
                                            toolCall = {
                                                {"id", ""},
                                                {"type", "function"},
                                                {"function", {
                                                    {"name", ""},
                                                    {"arguments", ""}
                                                }}
                                            };
                                        }
                                        
                                        // Actualitzar ID
                                        if (toolCallDelta.contains("id")) {
                                            toolCall["id"] = toolCallDelta["id"];
                                        }
                                        
                                        // Actualitzar nom de la funció
                                        if (toolCallDelta.contains("function") && 
                                            toolCallDelta["function"].contains("name")) {
                                            toolCall["function"]["name"] = toolCallDelta["function"]["name"];
                                        }
                                        
                                        // Actualitzar arguments (acumular)
                                        if (toolCallDelta.contains("function") && 
                                            toolCallDelta["function"].contains("arguments")) {
                                            string newArgs = toolCallDelta["function"]["arguments"];
                                            toolCall["function"]["arguments"] = 
                                                toolCall["function"]["arguments"].get<string>() + newArgs;
                                        }
                                        
                                        streamData->inToolCall = true;
                                    }
                                }
                            }
                        }
                    } catch (const json::exception& e) {
                        // Ignorar errors de parsing en streaming
                    }
                }
            }
        }
        
        return totalSize;
    }


public:
    DeepSeekClient(const string& apiKey = "", const string& baseUrl = "https://api.deepseek.com")
        : baseUrl(baseUrl), model("deepseek-chat"), maxChatHistory(1000), onCommandResult(nullptr) {
        lastCommandResults.clear();
        pendingCommandResults.clear();
        if (!apiKey.empty()) {
            this->apiKey = apiKey;
        } else {
            const char* envKey = getenv("DEEPSEEK_API_KEY");
            if (envKey) {
                this->apiKey = envKey;
            } else {
                throw runtime_error("API key not found. Define DEEPSEEK_API_KEY environment variable");
            }
        }
        
        terminal = make_unique<TerminalEmulator>();
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }
    
    ~DeepSeekClient() {
        // Netejar el callback
        if (onCommandResult) {
            delete onCommandResult;
            onCommandResult = nullptr;
        }
        curl_global_cleanup();
    }

    void setOnCommandResultCallback(function<void(const json&)> callback) {
        if (onCommandResult) {
            delete onCommandResult;
        }
        onCommandResult = new function<void(const json&)>(callback);
    }

    pair<string, vector<json>> chatWithToolsStreaming(
        const string& userMessage, 
        function<void(const string&)> onChunk,
        bool useTools = true,
        float temperature = 0.7,
        int maxTokens = 8000,
        int maxIterations = 100) {
        
        vector<json> messages;
        string fullResponse = "";
        int iteration = 0;
        
        // Add system prompt
        string systemPrompt = "Ets un assistent AI útil que parla català. Pots executar comandes de terminal quan sigui necessari. No usis emoticones.";
        
        // Intentar llegir el fitxer prompt.md del directori actual
        string promptFilePath = terminal->getCurrentDir() + "/prompt.md";
        ifstream promptFile(promptFilePath);
        
        if (promptFile.is_open()) {
            stringstream buffer;
            buffer << promptFile.rdbuf();
            string customPrompt = buffer.str();
            
            if (!customPrompt.empty()) {
                // Afegir el contingut del prompt.md al system prompt
                systemPrompt = systemPrompt + "\n\n" +  customPrompt;
                cout << "CUSTOM PROMPT: " << customPrompt << endl;
            }
            promptFile.close();
        }
        
        messages.push_back({{"role", "system"}, {"content", systemPrompt}});        // Add chat history

        for (const auto& msg : chatHistory) {
            messages.push_back(msg);
        }
        
        // Add current user message
        messages.push_back({{"role", "user"}, {"content", userMessage}});
        addToChatHistory("user", userMessage);
        
        while (iteration < maxIterations) {
            iteration++;
            
            // Prepare request
            json request = {
                {"model", model},
                {"messages", messages},
                {"stream", true},
                {"temperature", temperature},
                {"max_tokens", maxTokens}
            };
            
            if (useTools) {
                auto tools = getAvailableTools();
                if (!tools.empty()) {
                    request["tools"] = tools;
                    request["tool_choice"] = "auto";
                }
            }
            
            CURL* curl = curl_easy_init();
            if (!curl) {
                onChunk("Error: Unable to initialize cURL");
                return {fullResponse, messages};
            }
            
            string url = baseUrl + "/chat/completions";
            
            // Crear StreamData amb el callback
            StreamData* streamData = new StreamData();
            streamData->onChunk = new function<void(const string&)>(onChunk);
            
            struct curl_slist* headers = NULL;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            headers = curl_slist_append(headers, ("Authorization: Bearer " + apiKey).c_str());
            headers = curl_slist_append(headers, "Accept: text/event-stream");
            
            string requestBody = request.dump();
            
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBody.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, requestBody.size());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, StreamingWriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, streamData);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);
            
            // Configurar per streaming
            curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 1024);
            curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1L);
            
            CURLcode res = curl_easy_perform(curl);
            
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            
            // Guardar la resposta completa
            fullResponse = streamData->fullResponse;
            
            // Netejar tool calls buits
            vector<json> validToolCalls;
            for (const auto& toolCall : streamData->toolCalls) {
                if (!toolCall.is_null() && !toolCall["function"]["name"].get<string>().empty()) {
                    validToolCalls.push_back(toolCall);
                }
            }
            
            // Alliberar memòria
            delete streamData->onChunk;
            delete streamData;
            
            if (res != CURLE_OK) {
                onChunk("Error: " + string(curl_easy_strerror(res)));
                return {fullResponse, messages};
            }
            
            // Si hi ha tool calls, processar-les
            if (!validToolCalls.empty()) {
                // Afegir missatge de l'assistent amb tool calls
                json assistantMessage = {
                    {"role", "assistant"},
                    {"content", fullResponse}
                };
                
                if (!validToolCalls.empty()) { 
                    assistantMessage["tool_calls"] = validToolCalls; 
                }
                
                messages.push_back(assistantMessage);
                auto toolResults = processToolCalls(validToolCalls);
                
                // Afegir resultats de tools als missatges
                for (const auto& result : toolResults) {
                    messages.push_back(result);
                }
                
                // Continuar el bucle per la següent iteració
                continue;
            }
            
            // No hi ha més tool calls, sortir del bucle
            break;
        }
        
        return {fullResponse, messages};
    }

    void chatStream(const string& userMessage, function<void(const string&)> onChunk, bool useTools = false) {
        auto [fullResponse, updatedMessages] = chatWithToolsStreaming(userMessage, onChunk, useTools);
        
        if (!fullResponse.empty()) {
            addToChatHistory("assistant", fullResponse);
        }
    }

    vector<json> getAvailableTools() {
        return {
            {
                {"type", "function"},
                {"function", {
                    {"name", "shell_exec"},
                    {"description", "Executa una comanda al terminal gnu/linux. Utilitzala per realitzar tasques al sistema."},
                    {"parameters", {
                        {"type", "object"},
                        {"properties", {
                            {"command", {{"type", "string"}, {"description", "Comanda a executar (ex: 'ls -la', 'pwd', 'cat fitxer.txt')"}}},
                            {"explanation", {{"type", "string"}, {"description", "Explicació de què fa la comanda i per què s'executa, quina es la intenció"}}}
                        }},
                        {"required", {"command", "explanation"}}
                    }}
                }}
            }
        };
    }
    
    json handleShellExec(const json& args) {
        string command = args.value("command", "");
        string explanation = args.value("explanation", "");
        
        auto result = terminal->executeCommand(command);
        
        json formattedResult = {
            {"status", result.success ? "success" : "error"},
            {"command", command},
            {"returnCode", result.returnCode},
            {"stdout", result.stdout},
            {"stderr", result.stderr},
            {"explanation", explanation},
            {"currentDirectory", terminal->getCurrentDir()}
        };
        
        // Mostrar el resultat immediatament si hi ha callback
        if (onCommandResult) {
            try {
                (*onCommandResult)(formattedResult);
            } catch (const bad_function_call& e) {
                // Ignorar si el callback no és vàlid
            }
        }
        
        return formattedResult;
    }

    vector<json> processToolCalls(const vector<json>& toolCalls) {
        vector<json> toolResults;
        lastCommandResults.clear(); // Netejar resultats anteriors
        pendingCommandResults.clear(); // Netejar resultats pendents
        
        for (const auto& toolCall : toolCalls) {
            if (toolCall.is_null()) continue;
            
            string functionName = toolCall["function"]["name"];
            string argumentsStr = toolCall["function"]["arguments"];
            
            json result;
            
            try {
                json functionArgs = json::parse(argumentsStr);
                
                if (functionName == "shell_exec") {
                    result = handleShellExec(functionArgs);
                    // Guardar el resultat per mostrar-lo després
                    lastCommandResults.push_back(result);
                    pendingCommandResults.push_back(result); // Afegir als pendents
                } else {
                    result = {{"error", "Unknown function: " + functionName}, {"status", "error"}};
                }
            } catch (const json::exception& e) {
                result = {{"error", "Error parsing arguments: " + string(e.what())}, {"status", "error"}};
            }
            
            // Envoltar el resultat en un bloc de codi JSON per facilitar el parsing
            string content = "```json\n" + result.dump(2) + "\n```";
            
            toolResults.push_back({
                {"tool_call_id", toolCall["id"]},
                {"role", "tool"},
                {"name", functionName},
                {"content", content}
            });
        }
        
        return toolResults;
    }
    
    vector<json> getLastCommandResults() const {
        return lastCommandResults;
    }
    
    // Nou mètode: obtenir i netejar resultats pendents
    vector<json> getAndClearPendingCommandResults() {
        vector<json> results = pendingCommandResults;
        pendingCommandResults.clear();
        return results;
    }

    void clearChatHistory() {
        chatHistory.clear();
    }
    
    void addToChatHistory(const string& role, const string& content) {
        chatHistory.push_back({{"role", role}, {"content", content}});
        if (chatHistory.size() > maxChatHistory) {
            chatHistory.erase(chatHistory.begin());
        }
    }

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }
    
    string getCurrentDirectory() const {
        return terminal->getCurrentDir();
    }
    
    bool changeDirectory(const string& dir) {
        return terminal->setCurrentDir(dir);
    }
};

// Async task manager for non-blocking API calls
class AsyncTaskManager {
private:
    struct StreamingTask {
        string message;
        bool useTools;
        function<void(const string&)> onChunk;
        function<void()> onComplete;
    };
    
    queue<StreamingTask> streamingQueue;
    mutex streamingMutex;
    condition_variable streamingCondition;
    thread streamingThread;
    atomic<bool> streamingRunning;
    unique_ptr<DeepSeekClient> deepSeekClient;
 
    void streamingWorkerFunction() {
        while (streamingRunning) {
            StreamingTask task;
            
            {
                unique_lock<mutex> lock(streamingMutex);
                streamingCondition.wait(lock, [this]() {
                    return !streamingQueue.empty() || !streamingRunning;
                });
                
                if (!streamingRunning && streamingQueue.empty()) {
                    break;
                }
                
                if (!streamingQueue.empty()) {
                    task = move(streamingQueue.front());
                    streamingQueue.pop();
                }
            }
            
            if (!task.message.empty() && deepSeekClient) {
                try {
                    // Utilitzar el nou mètode que processa tool calls
                    deepSeekClient->chatStream(task.message, task.onChunk, task.useTools);
                    if (task.onComplete) {
                        task.onComplete();
                    }
                } catch (const exception& e) {
                    if (task.onChunk) {
                        task.onChunk("Error: " + string(e.what()));
                    }
                }
            }
        }
    }

public:
    AsyncTaskManager() : streamingRunning(true) {
        try {
            deepSeekClient = make_unique<DeepSeekClient>();
            streamingThread = thread(&AsyncTaskManager::streamingWorkerFunction, this);
        } catch (const exception& e) {
            streamingRunning = false;
            throw;
        }
    }
    
    ~AsyncTaskManager() {
        streamingRunning = false;
        streamingCondition.notify_all();
        if (streamingThread.joinable()) {
            streamingThread.join();
        }
    }

    void setOnCommandResultCallback(function<void(const json&)> callback) {
        if (deepSeekClient) {
            deepSeekClient->setOnCommandResultCallback(callback);
        }
    }

    void submitStreamingTask(const string& message, 
                            function<void(const string&)> onChunk,
                            function<void()> onComplete,
                            bool useTools = true) {
        StreamingTask task;
        task.message = message;
        task.useTools = useTools;
        task.onChunk = onChunk;
        task.onComplete = onComplete;
        
        {
            lock_guard<mutex> lock(streamingMutex);
            streamingQueue.push(move(task));
        }
        
        streamingCondition.notify_one();
    }
    
    void clearChatHistory() {
        if (deepSeekClient) {
            deepSeekClient->clearChatHistory();
        }
    }
    
    bool changeDirectory(const string& dir) {
        if (deepSeekClient) {
            return deepSeekClient->changeDirectory(dir);
        }
        return false;
    }
    
    string getCurrentDirectory() const {
        if (deepSeekClient) {
            return deepSeekClient->getCurrentDirectory();
        }
        return "";
    }
    
    bool isInitialized() const {
        return deepSeekClient != nullptr;
    }
    
    vector<json> getLastCommandResults() const {
        if (deepSeekClient) {
            return deepSeekClient->getLastCommandResults();
        }
        return {};
    }

    vector<json> getAndClearPendingCommandResults() {
        if (deepSeekClient) {
            return deepSeekClient->getAndClearPendingCommandResults();
        }
        return {};
    }
};

// Main application class
class ChatApplication {
private:
    // Comptador de tokens per a la consulta actual
    mutable int currentQueryTokenCount;
    mutable mutex tokenCountMutex;
    mutable int sessionTotalTokens;
    mutable int sessionTotalBytes;

public:
    // Obtenir el comptador de tokens de la consulta actual
    int getCurrentQueryTokenCount() const {
        lock_guard<mutex> lock(tokenCountMutex);
        return currentQueryTokenCount;
    }
    
    // Obtenir estadístiques de sessió
    pair<int, int> getSessionStats() const {
        lock_guard<mutex> lock(tokenCountMutex);
        return {sessionTotalTokens, sessionTotalBytes};
    }
    
    // Reiniciar el comptador de tokens
    void resetTokenCount() {
        lock_guard<mutex> lock(tokenCountMutex);
        currentQueryTokenCount = 0;
    }
    
    // Afegir tokens i bytes a la sessió
    void addToSessionStats(int tokens, int bytes) {
        lock_guard<mutex> lock(tokenCountMutex);
        sessionTotalTokens += tokens;
        sessionTotalBytes += bytes;
    }
    
private:
    unique_ptr<AsyncTaskManager> asyncTaskManager;
    char textBuffer[16384];
    bool isInitialized;
    bool isProcessingTask;
    string currentProcessingMessage;
    bool requestFocusAfterResponse;
    bool toolsEnabled;
    string streamingResponse;
    bool isStreaming;
    
    // Mutex per protegir l'accés a les llistes de missatges
    mutable mutex messagesMutex;
    mutable mutex pendingResultsMutex;
    vector<json> pendingCommandResults; // Resultats pendents de mostrar

    struct CommandOutputState {
        bool isExpanded;
        string commandId; // Identificador únic per a cada comanda
    };
    
    vector<CommandOutputState> commandOutputStates;
    int nextCommandId;

    
    
    


static void renderMarkdownText(const string& text, bool isStreaming = false) {
    if (text.empty()) return;
    
    // Separar el text per línies
    vector<string> lines;
    stringstream ss(text);
    string line;
    
    while (getline(ss, line)) {
        lines.push_back(line);
    }
    
    // Estat per controlar si estem dins d'un bloc de codi
    bool inCodeBlock = false;
    string codeBlockLanguage = "";
    int codeLineNumber = 1; // Comptador de línies dins del bloc de codi
    
    // Processar cada línia
    for (size_t i = 0; i < lines.size(); i++) {
        string currentLine = lines[i];
        
        // Verificar si és l'inici o final d'un bloc de codi
        if (currentLine.find("```") == 0) {
            if (!inCodeBlock) {
                // Inici d'un bloc de codi
                inCodeBlock = true;
                codeBlockLanguage = currentLine.substr(3); // Pot contenir el llenguatge
                codeLineNumber = 1; // Reiniciar comptador
                
                // NO renderitzar la línia amb ``` - simplement saltar-la
                // Continuar al següent element sense mostrar res
                continue;
            } else {
                // Final d'un bloc de codi
                inCodeBlock = false;
                codeBlockLanguage = "";
                codeLineNumber = 1; // Reiniciar comptador per al proper bloc
                
                // NO renderitzar la línia amb ``` - simplement saltar-la
                // Continuar al següent element sense mostrar res
                continue;
            }
        }
        
        // Si estem dins d'un bloc de codi, mostrar la línia amb estil de codi i número de línia
        if (inCodeBlock) {
            // Crear un contenidor per a la línia de codi
            ImGui::BeginGroup();
            
            // Número de línia (púrpura)
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.2f, 0.8f, 1.0f)); // Púrpura
            ImGui::Text("%3d", codeLineNumber); // Format: 3 dígits amb espais a l'esquerra
            ImGui::PopStyleColor();
            
            // Separador entre número i codi
            ImGui::SameLine(0, 10.0f); // 10px d'espai
            
            // Contingut del codi
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f)); // Text gris clar
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f)); // Fons negre
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 10.0f)); // Padding intern
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f); // Vores arrodonides
            
            // Calcular l'amplada disponible per al codi (restant l'espai del número de línia)
            float lineNumberWidth = ImGui::CalcTextSize("999").x + 10.0f; // Amplada per a 3 dígits + espai
            float availableWidth = ImGui::GetContentRegionAvail().x - lineNumberWidth;
            
            // Renderitzar el text del codi
            ImGui::TextWrapped("%s", currentLine.c_str());
            
            // Restaurar estils
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(2);
            
            ImGui::EndGroup();
            
            // Incrementar el número de línia per a la següent línia de codi
            codeLineNumber++;
            
            // Afegir un petit espai després de la línia de codi (només si no és l'última línia)
            if (i < lines.size() - 1) {
                ImGui::Spacing();
            }
            
            continue; // Saltar al processament de la següent línia
        }
        
        // Si no estem en un bloc de codi, processar Markdown normalment
        
        // Verificar si és una llista amb asterisc (*)
        bool isListItem = false;
        string listItemContent = "";
        
        // Eliminar espais inicials per detectar el patró
        size_t firstNonSpace = currentLine.find_first_not_of(" \t");
        if (firstNonSpace != string::npos) {
            string trimmedLine = currentLine.substr(firstNonSpace);
            
            if (trimmedLine.length() >= 2 && 
                trimmedLine[0] == '*' && 
                trimmedLine[1] == ' ') {
                // Verificar que hi hagi contingut després de l'asterisc
                size_t contentStart = 1;
                while (contentStart < trimmedLine.length() && trimmedLine[contentStart] == ' ') {
                    contentStart++;
                }
                
                if (contentStart < trimmedLine.length()) {
                    isListItem = true;
                    listItemContent = trimmedLine.substr(contentStart);
                }
            }
        }
        
        // Verificar si la línia està completament en negreta
        bool isBoldLine = false;
        string boldContent = "";
        
        // Patró: **text** (sense res més a la línia)
        if (currentLine.length() >= 4 && 
            currentLine.find("**") == 0) {
            size_t endBold = currentLine.find("**", 2);
            if (endBold != string::npos && 
                endBold == currentLine.length() - 2) {
                isBoldLine = true;
                boldContent = currentLine.substr(2, endBold - 2);
            }
        }
        
        // Verificar si és un títol (#, ##, ###, etc.)
        bool isTitle = false;
        int titleLevel = 0;
        string titleContent = "";
        
        // Comprovar diferents nivells de títol
        for (int level = 1; level <= 6; level++) {
            string prefix = string(level, '#') + " ";
            if (currentLine.length() >= prefix.length() && 
                currentLine.find(prefix) == 0) {
                isTitle = true;
                titleLevel = level;
                titleContent = currentLine.substr(prefix.length()); // Eliminar prefix
                break;
            }
        }
        
        // Verificar si és un separador (---)
        bool isSeparator = false;
        if (currentLine.length() >= 3) {
            // Eliminar espais al principi i final
            string trimmedLine = currentLine;
            size_t firstNonSpace = trimmedLine.find_first_not_of(" \t");
            size_t lastNonSpace = trimmedLine.find_last_not_of(" \t");
            
            if (firstNonSpace != string::npos && lastNonSpace != string::npos) {
                trimmedLine = trimmedLine.substr(firstNonSpace, lastNonSpace - firstNonSpace + 1);
                
                // Comprovar si és un separador (--- o ***)
                if (trimmedLine.length() >= 3 && 
                    (trimmedLine.find("---") == 0 || trimmedLine.find("***") == 0)) {
                    // Verificar que tots els caràcters siguin '-' o '*'
                    bool allSame = true;
                    char firstChar = trimmedLine[0];
                    for (char c : trimmedLine) {
                        if (c != firstChar) {
                            allSame = false;
                            break;
                        }
                    }
                    if (allSame && trimmedLine.length() >= 3) {
                        isSeparator = true;
                    }
                }
            }
        }
        
        // Netejar negretes del contingut del títol
        if (isTitle) {
            // Eliminar ** del principi i final si existeixen
            if (titleContent.length() >= 4 && 
                titleContent.find("**") == 0) {
                size_t endBold = titleContent.find("**", 2);
                if (endBold != string::npos && 
                    endBold == titleContent.length() - 2) {
                    titleContent = titleContent.substr(2, endBold - 2);
                }
            }
        }
        
        if (isListItem) {
            // Renderitzar element de llista amb bullet
            ImGui::BeginGroup();
            
            // Bullet amb una mica de padding
            ImGui::Bullet();
            ImGui::SameLine(0, 8.0f); // 8px d'espai entre el bullet i el text
            
            // Contingut de l'element de llista
            ImGui::TextWrapped("%s", listItemContent.c_str());
            
            ImGui::EndGroup();
        } else if (isBoldLine) {
            // Renderitzar en negreta - incrementar la mida de la font per simular negreta
            ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]); // Font regular
            
            // Establir nova escala per simular negreta
            ImGui::SetWindowFontScale(1.1f); // 10% més gran
            
            // Utilitzar TextColored en lloc de PushStyleColor/PopStyleColor
            // Així no interfereix amb el color del tipus de missatge
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", boldContent.c_str());
            
            // Restaurar l'escala per defecte
            ImGui::SetWindowFontScale(1.0f);
            
            ImGui::PopFont();
        } else if (isTitle) {
            // Renderitzar títol amb mida proporcional al nivell
            ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]); // Font regular
            
            // Configurar espai segons el nivell
            float verticalPadding = 10.0f - (titleLevel * 1.0f); // Més padding per títols superiors
            if (verticalPadding < 4.0f) verticalPadding = 4.0f;
            
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, verticalPadding));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 4));
            
            // Escalar la font segons el nivell del títol
            // # = 1.4, ## = 1.3, ### = 1.2, #### = 1.1, etc.
            float fontSizeScale = 1.5f - (titleLevel * 0.1f);
            if (fontSizeScale < 1.0f) fontSizeScale = 1.0f;
            
            ImGui::SetWindowFontScale(fontSizeScale);
            ImGui::TextWrapped("%s", titleContent.c_str());
            ImGui::SetWindowFontScale(1.0f);
            
            ImGui::PopStyleVar(2);
            ImGui::PopFont();
        } else if (isSeparator) {
            // Renderitzar separador
            ImGui::Separator();
        } else {
            // Renderitzar normalment
            ImGui::TextWrapped("%s", currentLine.c_str());
        }
        
        // Afegir un petit espai entre línies (excepte l'última)
        if (i < lines.size() - 1) {
            ImGui::Spacing();
        }
    }
}











public:
    // Estructura per emmagatzemar missatges amb tipus
    struct ChatMessage {
        string text;
        enum Type { USER, AI, COMMAND_OUTPUT, COMMAND_ERROR, SYSTEM } type;
    };
    
    string getCurrentDirectory() const {
        if (asyncTaskManager) {
            return asyncTaskManager->getCurrentDirectory();
        }
        return "N/A";
    }
private:
    vector<ChatMessage> chatMessages;


public:
    ChatApplication() : isInitialized(false), isProcessingTask(false), 
                       requestFocusAfterResponse(false), toolsEnabled(false),
                       isStreaming(false), nextCommandId(0),
                       currentQueryTokenCount(0),
                       sessionTotalTokens(0), sessionTotalBytes(0) {  // NOU: Inicialitzar comptadors de sessió
        memset(textBuffer, 0, sizeof(textBuffer));
    }
    
    ~ChatApplication() {
    }
 
    // Funció per renderitzar text amb Markdown
    static void renderTextWithMarkdown(const string& text, bool isStreaming = false) {
        // Si està buit, no fer res
        if (text.empty()) return;
        
        // Separar el prefix "IA: " si existeix
        string displayText = text;
        bool hasPrefix = false;
        if (text.find("IA: ") == 0) {
            displayText = text.substr(4);
            hasPrefix = true;
        } else if (text.find("Tu: ") == 0) {
            displayText = text.substr(4);
            hasPrefix = true;
        }
        
        // Renderitzar prefix si n'hi ha
        if (hasPrefix) {
            string prefix = text.substr(0, 4);
            ImGui::TextWrapped("%s", prefix.c_str());
            ImGui::SameLine(0, 0);
        }
        
        renderMarkdownText(displayText, isStreaming);
        
        // Afegir cursor si està en streaming
        if (isStreaming) {
            ImGui::SameLine(0, 0);
            ImGui::Bullet();
        }
    }


    void addPendingCommandResult(const json& result) {
        lock_guard<mutex> lock(pendingResultsMutex);
        pendingCommandResults.push_back(result);
    }
    
    void processPendingCommandResults() {
        vector<json> results;
        {
            lock_guard<mutex> lock(pendingResultsMutex);
            results = move(pendingCommandResults);
            pendingCommandResults.clear();
        }
        
        for (const auto& result : results) {
            showCommandResultImmediately(result);
        }
    }
    
    
    void showCommandResultImmediately(const json& result) {
        lock_guard<mutex> lock(messagesMutex);
        
        if (result.contains("status") && result.contains("command")) {
            // Generar un ID únic per a aquesta comanda
            string commandId = "cmd_" + to_string(nextCommandId++);
            
            // Crear estat inicial (expandit per defecte)
            commandOutputStates.push_back({false, commandId});
            
            // Crear el missatge amb l'estructura especial per a comandes
            ChatMessage cmdMessage;
            cmdMessage.type = ChatMessage::COMMAND_OUTPUT;
            cmdMessage.text = commandId + "|" + result.dump(); // Emmagatzemar l'ID i les dades JSON
            chatMessages.push_back(cmdMessage);
        }
    }

    
    bool initialize() {
        try {
            asyncTaskManager = make_unique<AsyncTaskManager>();
            
            // Modificar el callback per emmagatzemar resultats pendents
            asyncTaskManager->setOnCommandResultCallback([this](const json& result) {
                this->addPendingCommandResult(result);
            });
            
            isInitialized = true;
            addMessage("Type something...", ChatMessage::SYSTEM);
            return true;
        } catch (const exception& e) {
            addMessage("Unable to chat: " + string(e.what()), ChatMessage::SYSTEM);
            addMessage("DEEPSEEK_API_KEY environ variable is setted?", ChatMessage::SYSTEM);
            return false;
        }
    }

    void addMessage(const string& text, ChatMessage::Type type) {
        lock_guard<mutex> lock(messagesMutex);
        chatMessages.push_back({text, type});
    }

    void sendMessage(const string& message) {
        if (!isInitialized || message.empty() || isProcessingTask) return;
        
        lock_guard<mutex> lock(tokenCountMutex);
        currentQueryTokenCount = TokenCounter::countTokens(message);

        // Afegir tokens i bytes del missatge d'usuari a la sessió
        sessionTotalTokens += TokenCounter::countTokens(message);
        sessionTotalBytes += message.length();

        addMessage("Tu: " + message, ChatMessage::USER);
        
        // Crear un missatge d'IA buit que anirem actualitzant
        streamingResponse = "";
        isStreaming = true;
        addMessage("IA: ", ChatMessage::AI);
        
        currentProcessingMessage = message;
        isProcessingTask = true;
        requestFocusAfterResponse = true;
        
        // Contador per saber quants chunks hem rebut
        int chunkCounter = 0;
        // Utilitzar shared_ptr per compartir els comptadors entre lambdas
        auto totalResponseTokens = make_shared<int>(0);
        auto totalResponseBytes = make_shared<int>(0);
        
        // Utilitzar streaming amb tools
        asyncTaskManager->submitStreamingTask(
            message,
            [this, chunkCounter, totalResponseTokens, totalResponseBytes](const string& chunk) mutable {
                // Aquest callback s'executa quan arriba un chunk
                chunkCounter++;
                cout << "[CHAT] Chunk " << chunkCounter << " obtained: '" << chunk << "'" << endl;

                int chunkTokens = TokenCounter::countTokens(chunk);
                *totalResponseTokens += chunkTokens;
                *totalResponseBytes += chunk.length();
                {
                    lock_guard<mutex> lock(tokenCountMutex);
                    currentQueryTokenCount = TokenCounter::countTokens(currentProcessingMessage) + *totalResponseTokens;
                }

                // Afegir el chunk a la resposta acumulada
                streamingResponse += chunk;
                
                // Actualitzar l'últim missatge d'IA
                {
                    lock_guard<mutex> lock(messagesMutex);
                    if (!chatMessages.empty()) {
                        // Buscar l'últim missatge d'IA
                        for (int i = chatMessages.size() - 1; i >= 0; i--) {
                            if (chatMessages[i].type == ChatMessage::AI) {
                                // Actualitzar el text del missatge
                                chatMessages[i].text = "IA: " + streamingResponse;
                                break;
                            }
                        }
                    }
                }
                
                // Processar qualsevol resultat de comanda pendent
                processPendingCommandResults();
            },
            [this, totalResponseTokens, totalResponseBytes]() {
                // Aquest callback s'executa quan acaba el stream
                isStreaming = false;
                isProcessingTask = false;
                {
                    lock_guard<mutex> lock(tokenCountMutex);
                    currentQueryTokenCount = TokenCounter::countTokens(currentProcessingMessage) + *totalResponseTokens;
                    // Afegir tokens i bytes de la resposta a la sessió
                    sessionTotalTokens += *totalResponseTokens;
                    sessionTotalBytes += *totalResponseBytes;
                }

                // Si la resposta està buida, afegir un missatge indicant que s'ha executat una comanda
                if (streamingResponse.empty()) {
                    lock_guard<mutex> lock(messagesMutex);
                    if (!chatMessages.empty() && 
                        chatMessages.back().type == ChatMessage::AI) {
                        chatMessages.back().text = "IA: [He executat una comanda de terminal]";
                    }
                }
                
                // Processar qualsevol resultat pendent restant
                processPendingCommandResults();
                
                currentProcessingMessage.clear();
            },
            toolsEnabled
        );
    }

    bool getIsStreaming() const { return isStreaming; }

    void update() {
        processPendingCommandResults();
    }

    bool shouldRequestFocus() const { return requestFocusAfterResponse; }
    void resetFocusRequest() { requestFocusAfterResponse = false; }
    
    void clearChat() {
        if (asyncTaskManager) {
            asyncTaskManager->clearChatHistory();
        }
        {
            lock_guard<mutex> lock(messagesMutex);
            chatMessages.clear();
            commandOutputStates.clear();
            nextCommandId = 0;
        }
        {
            lock_guard<mutex> lock(pendingResultsMutex);
            pendingCommandResults.clear();
        }
        // Netejar el comptador de tokens
        {
            lock_guard<mutex> lock(tokenCountMutex);
            currentQueryTokenCount = 0;
        }
        addMessage("Type something to start...", ChatMessage::SYSTEM);
        
        // Cancel any pending streaming task
        if (isProcessingTask) {
            isProcessingTask = false;
            isStreaming = false;
            currentProcessingMessage.clear();
        }
        requestFocusAfterResponse = true;
    }

    void changeDirectory(const string& dir) {
        if (asyncTaskManager && asyncTaskManager->changeDirectory(dir)) {
            addMessage("Folder changed: " + asyncTaskManager->getCurrentDirectory(), ChatMessage::SYSTEM);
        } else {
            addMessage("Unable to change to: " + dir, ChatMessage::SYSTEM);
        }
    }

    vector<ChatMessage> getChatMessages() const {
        lock_guard<mutex> lock(messagesMutex);
        return chatMessages;
    }

    bool isCommandExpanded(const string& commandId) const {
        lock_guard<mutex> lock(messagesMutex);
        for (const auto& state : commandOutputStates) {
            if (state.commandId == commandId) {
                return state.isExpanded;
            }
        }
        return true; // Per defecte, expandit
    }
    
void toggleCommandExpanded(const string& commandId) {
    lock_guard<mutex> lock(messagesMutex);
    for (auto& state : commandOutputStates) {
        if (state.commandId == commandId) {
            state.isExpanded = !state.isExpanded;
            break;
        }
    }
}
    char* getTextBuffer() { return textBuffer; }
    size_t getTextBufferSize() const { return sizeof(textBuffer); }
    bool getIsInitialized() const { return isInitialized; }
    bool getIsProcessingTask() const { return isProcessingTask; }
    
    bool getToolsEnabled() const { return toolsEnabled; }
    void setToolsEnabled(bool enabled) { toolsEnabled = enabled; }

    // Clipboard
    string getAllChatText() const {
        string allText;
        for (const auto& message : chatMessages) {
            allText += message.text + "\n";
        }
        return allText;
    }
};


int main(int argc, char *argv[]) {
    // Initialize GLFW
    if (!glfwInit()) {
        return 1;
    }
    // Configure OpenGL version
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    float mainScale = 1.0f;
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    if (primaryMonitor) {
        float xscale, yscale;
        glfwGetMonitorContentScale(primaryMonitor, &xscale, &yscale);
        mainScale = (xscale + yscale) / 2.0f;
    }
    
    GLFWwindow* window = glfwCreateWindow(
        (int)(400 * mainScale), 
        (int)(600 * mainScale), 
        "", 
        nullptr, nullptr
    );
    
    if (window == nullptr) {
        glfwTerminate();
        return 2;
    }
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();


    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    /*
    // Font monospace amb suport Unicode
    ImFont* fontMono = nullptr;
    const char* monoPaths[] = {
        "/usr/share/fonts/truetype/noto/NotoMono-Regular.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
        "/usr/share/fonts/truetype/ubuntu/UbuntuMono-R.ttf",
        nullptr
    };

    for (int i = 0; monoPaths[i] != nullptr; i++) {
        if (access(monoPaths[i], F_OK) != -1) {
            fontMono = io.Fonts->AddFontFromFileTTF(monoPaths[i], 14.0f);
            if (fontMono) break;
        }
    }

    if (!fontMono) fontMono = io.Fonts->AddFontDefault();
    */

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    ImGuiStyle& style = ImGui::GetStyle();
    // Arrodonir botons
    style.FrameRounding = 8.0f;
    // Arrodonir camps de text
    style.FrameBorderSize = 1.0f;
    // Arrodonir finestres
    style.WindowRounding = 10.0f;
    // Arrodonir popups
    style.PopupRounding = 8.0f;
    // Arrodonir elements de scrollbar
    style.ScrollbarRounding = 8.0f;
    // Arrodonir sliders
    style.GrabRounding = 8.0f;
    // Arrodonir tabs
    style.TabRounding = 8.0f;
    // Arrodonir child windows
    style.ChildRounding = 8.0f;

    ImVec4 bgColor = ImVec4(0.15f, 0.18f, 0.25f, 1.0f);
    //style.Colors[ImGuiCol_WindowBg] = bgColor;
    //style.Colors[ImGuiCol_ChildBg] = bgColor;
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.23f, 0.30f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.28f, 0.35f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.33f, 0.40f, 1.0f);

    // Create chat 
    // Create chat application
    ChatApplication chatApp;
    bool appInitialized = chatApp.initialize();
    
    static float chatInputHeight = 40.0f;
    static bool setFocusToInput = true;
    
    
    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Process events
        glfwPollEvents();
        // Update chat application (check for async task completion)
        chatApp.update();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        
        ImGui::Begin("laIAUI", nullptr, 
            ImGuiWindowFlags_NoTitleBar | 
            ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoMove | 
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_MenuBar
        );
        
        // Menu bar
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Clean", "Ctrl+N")) {
                    chatApp.clearChat();
                }
                if (ImGui::MenuItem("Quit", "Ctrl+Q")) {
                    glfwSetWindowShouldClose(window, true);
                }
                ImGui::EndMenu();
            }

            // Status indicator
            ImGui::SameLine(100);
            //ImGui::SameLine(ImGui::GetWindowWidth() - 120);
            ImGui::Bullet();
            if (appInitialized) {
                if (chatApp.getIsProcessingTask()) {
                    // Crear efecto de parpadeo basado en el tiempo
                    static float blinkTimer = 0.0f;
                    blinkTimer += ImGui::GetIO().DeltaTime;
                    
                    // Parpadeo cada 0.5 segundos (2 Hz)
                    float blinkCycle = fmod(blinkTimer, 1.0f);
                    float alpha = (blinkCycle < 0.5f) ? 1.0f : 0.3f;
                    
                    // Obtenir el comptador de tokens actual
                    int tokenCount = chatApp.getCurrentQueryTokenCount();
                    
                    // Mostrar "Thinking..." amb el comptador de tokens
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, alpha), "Thinking (%d)...", tokenCount);
                } else {
                    // Obtenir estadístiques de sessió
                    auto [sessionTokens, sessionBytes] = chatApp.getSessionStats();
                    // Mostrar "Online" amb estadístiques de sessió
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Online (%d tokens, %d bytes)", 
                                      sessionTokens, sessionBytes);
                }
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Offline");
            }
            
            ImGui::EndMenuBar();
        }
        
        // Get window size
        ImVec2 windowSize = ImGui::GetWindowSize();
        // Calcular l'alçada disponible per al ChatArea
        // Restem: 
        // 1. Alçada de l'entrada de text (chatInputHeight)
        // 2. Separador
        // 3. Checkbox i tooltip
        // 4. Padding extra
        float menuBarHeight = ImGui::GetFrameHeight();
        float separatorHeight = ImGui::GetFrameHeight() * 0.5f;
        float checkboxHeight = ImGui::GetFrameHeight();
        float padding = ImGui::GetStyle().WindowPadding.y * 2;
        float extraSpacing = 20.0f;

        float availableHeight = windowSize.y 
        - menuBarHeight 
        - separatorHeight 
        - chatInputHeight 
        - separatorHeight 
        - checkboxHeight 
        - padding
        - extraSpacing;
        
        // Assegurar-nos que l'alçada no sigui negativa
        if (availableHeight < 50.0f) {
            availableHeight = 50.0f;
        }
        // Chat area
        ImGui::BeginChild("ChatArea", ImVec2(0, availableHeight), false, ImGuiWindowFlags_None);
        
        // Afegir menú contextual al ChatArea
        if (ImGui::BeginPopupContextWindow("ChatContextMenu")) {
            if (ImGui::MenuItem("Copy all")) {
                string allChatText = chatApp.getAllChatText();
                ImGui::SetClipboardText(allChatText.c_str());
            }
            ImGui::EndPopup();
        }
        

        // Display chat messages with colors
        const auto messages = chatApp.getChatMessages();
        for (size_t i = 0; i < messages.size(); i++) {
            const auto& message = messages[i];
            
            // Verificar si és un missatge de sortida de comanda amb estructura especial
            if (message.type == ChatApplication::ChatMessage::COMMAND_OUTPUT && 
                message.text.find("cmd_") == 0) {
                
                // Separar l'ID de la comanda i les dades JSON
                size_t separatorPos = message.text.find('|');
                if (separatorPos != string::npos) {
                    string commandId = message.text.substr(0, separatorPos);
                    string jsonData = message.text.substr(separatorPos + 1);
                    
                    try {
                        json result = json::parse(jsonData);
                        if (result.contains("explanation") && !result["explanation"].get<string>().empty()) {
                            string explanation = result["explanation"];
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
                            ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]); // Font regular
                            ImGui::TextWrapped("> %s", explanation.c_str());
                            ImGui::PopFont();
                            ImGui::PopStyleColor();
                            ImGui::Spacing();
                        }
                        
                        // Obtenir l'estat de col·lapse
                        bool isExpanded = chatApp.isCommandExpanded(commandId);
                        
                        // Crear l'encapçalament col·lapsable
                        string headerLabel = "cmd: " + result["command"].get<string>();
                        
                        // **Afegir identificador únic amb PushID**
                        ImGui::PushID(commandId.c_str());
                        
                        // Utilitzar CollapsingHeader amb bandera per detectar clics
                        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
                        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.4f, 0.4f, 0.4f, 0.7f));
                        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.5f, 0.5f, 0.5f, 0.9f));
                        
                        // ** Utilitzar una variable local per al resultat del CollapsingHeader**
                        bool headerClicked = false;
                        
                        // ** Renderitzar el CollapsingHeader amb l'estat correcte**
                        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
                        if (isExpanded) {
                            flags |= ImGuiTreeNodeFlags_DefaultOpen;
                        }

                        bool headerOpen = ImGui::CollapsingHeader(headerLabel.c_str(), flags);
                        
                        ImGui::PopStyleColor(3);
                        
                        // **Actualitzar l'estat només quan hi ha un canvi**
                        if (headerOpen != isExpanded) {
                            chatApp.toggleCommandExpanded(commandId);
                        }
                        
                        // ** Mostrar el contingut SI l'header està obert**
                        if (headerOpen) {
                            ImGui::Indent();
                            
                            // Mostrar stdout en gris
                            if (result.contains("stdout") && !result["stdout"].get<string>().empty()) {
                                string stdoutText = result["stdout"];
                                if (!stdoutText.empty() && stdoutText.back() == '\n') {
                                    stdoutText.pop_back();
                                }
                                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
                                ImGui::TextWrapped("%s", stdoutText.c_str());
                                ImGui::PopStyleColor();
                            }
                            
                            // Mostrar stderr en vermell
                            if (result.contains("stderr") && !result["stderr"].get<string>().empty()) {
                                string stderrText = result["stderr"];
                                if (!stderrText.empty() && stderrText.back() == '\n') {
                                    stderrText.pop_back();
                                }
                                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
                                ImGui::TextWrapped("%s", stderrText.c_str());
                                ImGui::PopStyleColor();
                            }
                            
                            // Mostrar informació addicional
                            ImGui::Spacing();
                            ImGui::Separator();
                            ImGui::Spacing();
                            
                            // Directori actual
                            if (result.contains("currentDirectory")) {
                                string dir = result["currentDirectory"];
                                ImGui::Text("path: %s", dir.c_str());
                            }
                            
                            // Estat i codi de retorn a la mateixa línia
                            if (result.contains("status")) {
                                string status = result["status"];
                                ImVec4 statusColor = (status == "success") ? 
                                    ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : 
                                    ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
                                ImGui::TextColored(statusColor, "Status: %s", status.c_str());
                                
                                // Afegir codi de retorn a la mateixa línia
                                ImGui::SameLine();
                                if (result.contains("returnCode")) {
                                    int returnCode = result["returnCode"];
                                    ImGui::Text("ret: %d", returnCode);
                                }
                            } else if (result.contains("returnCode")) {
                                // Si només hi ha codi de retorn sense estat
                                int returnCode = result["returnCode"];
                                ImGui::Text("ret: %d", returnCode);
                            }
                            
                            ImGui::Unindent();
                        }
                        
                        ImGui::PopID();
                        
                    } catch (const json::exception& e) {
                        ImGui::TextWrapped("%s", message.text.c_str());
                    }
                    
                    // Saltar al següent missatge
                    continue;
                }
            }
            
            // Per als altres tipus de missatges, mostrar normalment
            switch (message.type) {
                case ChatApplication::ChatMessage::USER:
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.8f, 1.0f, 1.0f));  // Blau clar
                    break;
                case ChatApplication::ChatMessage::AI:
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.9f, 0.3f, 1.0f));  // Verd groguenc
                    break;
                case ChatApplication::ChatMessage::COMMAND_OUTPUT:
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));  // Gris
                    break;
                case ChatApplication::ChatMessage::COMMAND_ERROR:
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));  // Vermell
                    break;
                case ChatApplication::ChatMessage::SYSTEM:
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.8f, 0.5f, 1.0f));  // Verd clar
                    break;
            }

            bool isLastMessage = (i == messages.size() - 1);
            bool isAIStreaming = (isLastMessage && 
                                message.type == ChatApplication::ChatMessage::AI &&
                                chatApp.getIsStreaming());        
            ChatApplication::renderTextWithMarkdown(message.text, isAIStreaming);

            ImGui::PopStyleColor();
            
            // Afegir un petit espai entre missatges
            ImGui::Spacing();
        }

        // Auto-scroll to bottom
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 10.0f) {
            ImGui::SetScrollHereY(1.0f);
        }

        ImGui::EndChild();
        
        // Separator
        ImGui::Dummy(ImVec2(0.0f, 5.0f));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0.0f, 5.0f));

        // Input area
        float textInputWidth = windowSize.x - ImGui::GetStyle().WindowPadding.x * 2 - 80.0f;
        
        // Give focus to input
        if (setFocusToInput) {
            ImGui::SetKeyboardFocusHere();
            setFocusToInput = false;
        }
        
        // Text input
        ImGui::PushItemWidth(textInputWidth);
        bool textEnterPressed = ImGui::InputTextMultiline(
            "##TextInput",
            chatApp.getTextBuffer(), 
            chatApp.getTextBufferSize(),
            ImVec2(textInputWidth, chatInputHeight),
            ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_EnterReturnsTrue
        );
        ImGui::PopItemWidth();
        
        // Send button
        ImGui::SameLine();
        bool sendButtonPressed = ImGui::Button("Send", ImVec2(70, chatInputHeight));

        // Separator
        //ImGui::Separator();

        // Tools checkbox area
        ImGui::Spacing();
        bool toolsEnabled = chatApp.getToolsEnabled();
        if (ImGui::Checkbox("Tools", &toolsEnabled)) {
            chatApp.setToolsEnabled(toolsEnabled); // Actualitzar quan canvia
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Allow launch shell commands");
            ImGui::EndTooltip();
        }

        // current path
        ImGui::SameLine();
        string currentDir = chatApp.getCurrentDirectory();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", currentDir.c_str());

        // Send message when Enter is pressed or Send button is clicked
        if ((textEnterPressed || sendButtonPressed) && chatApp.getTextBuffer()[0] != '\0' && !chatApp.getIsProcessingTask()) {
            string message = chatApp.getTextBuffer();
            chatApp.sendMessage(message);
            memset(chatApp.getTextBuffer(), 0, chatApp.getTextBufferSize());
            setFocusToInput = true; // Set flag to focus input on next frame
        }
        // Check if we should request focus after a response is complete
        if (chatApp.shouldRequestFocus()) {
            setFocusToInput = true;
            chatApp.resetFocusRequest();
        }
        
        ImGui::End();
        // Render
        ImGui::Render();
        
        // Clear screen
        int displayWidth, displayHeight;
        glfwGetFramebufferSize(window, &displayWidth, &displayHeight);
        glViewport(0, 0, displayWidth, displayHeight);
        glClearColor(float(37.0/255.0), float(42.0/255.0), float(82.0/255.0), 1.00f);
        //glClearColor(0.15f, 0.18f, 0.25f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Render ImGui
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        // Swap buffers
        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}
