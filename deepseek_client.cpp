#include "deepseek_client.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <mutex>

DeepSeekClient::DeepSeekClient(const string& apiKey, const string& baseUrl)
    : baseUrl(baseUrl), model("deepseek-chat"), cancelRequested(false), maxChatHistory(1000), currentCurlHandle(nullptr) , onCommandResult(nullptr) {
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

bool DeepSeekClient::isCancelRequested() const {
    return cancelRequested.load();
}

void DeepSeekClient::cancelCurrentRequest() {
    cancelRequested.store(true);
    lock_guard<mutex> lock(curlMutex);
    if (currentCurlHandle) {
        curl_easy_cleanup(currentCurlHandle);
        currentCurlHandle = nullptr;
    }
}

DeepSeekClient::~DeepSeekClient() {
    cancelCurrentRequest();
    if (onCommandResult) {
        delete onCommandResult;
        onCommandResult = nullptr;
    }
    curl_global_cleanup();
}

void DeepSeekClient::setOnCommandResultCallback(function<void(const json&)> callback) {
    if (onCommandResult) {
        delete onCommandResult;
    }
    onCommandResult = new function<void(const json&)>(callback);
}

pair<string, vector<json>> DeepSeekClient::chatWithToolsStreaming(
    const string& userMessage, 
    function<void(const string&)> onChunk,
    bool useTools,
    float temperature,
    int maxTokens,
    int maxIterations) {

    cancelRequested.store(false);

    vector<json> messages;
    string fullResponse = "";
    int iteration = 0;
    
    string systemPrompt = "Ets un assistent AI útil que parla català. Pots executar comandes de terminal quan sigui necessari. No usis emoticones.";
    
    string promptFilePath = terminal->getCurrentDir() + "/prompt.md";
    ifstream promptFile(promptFilePath);
    
    if (promptFile.is_open()) {
        stringstream buffer;
        buffer << promptFile.rdbuf();
        string customPrompt = buffer.str();
        
        if (!customPrompt.empty()) {
            systemPrompt = systemPrompt + "\n\n" +  customPrompt;
            cout << "CUSTOM PROMPT: " << customPrompt << endl;
        }
        promptFile.close();
    }
    
    messages.push_back({{"role", "system"}, {"content", systemPrompt}});
    for (const auto& msg : chatHistory) { messages.push_back(msg); }
    
    messages.push_back({{"role", "user"}, {"content", userMessage}});
    addToChatHistory("user", userMessage);
    
    while (iteration < maxIterations) {
        iteration++;
        
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
        {
            lock_guard<mutex> lock(curlMutex);
            currentCurlHandle = curl;
        }        
        string url = baseUrl + "/chat/completions";
        
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
        
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, 
            [](void* clientp, curl_off_t dltotal, curl_off_t dlnow,
            curl_off_t ultotal, curl_off_t ulnow) -> int {
                DeepSeekClient* client = static_cast<DeepSeekClient*>(clientp);
                return client->isCancelRequested() ? 1 : 0;
            });
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, this);
                
        curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 1024);
        curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1L);
        
        CURLcode res = curl_easy_perform(curl);
        {
            lock_guard<mutex> lock(curlMutex);
            currentCurlHandle = nullptr;
        }
            
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        if (cancelRequested.load()) {
            onChunk("\n\n[Cancel·lat per l'usuari]");
            return {fullResponse + "\n\n[Cancel·lat per l'usuari]", messages};
        }

        fullResponse = streamData->fullResponse;
        
        vector<json> validToolCalls;
        for (const auto& toolCall : streamData->toolCalls) {
            if (!toolCall.is_null() && !toolCall["function"]["name"].get<string>().empty()) {
                validToolCalls.push_back(toolCall);
            }
        }
        
        delete streamData->onChunk;
        delete streamData;
        
        if (res != CURLE_OK) {
            onChunk("Error: " + string(curl_easy_strerror(res)));
            return {fullResponse, messages};
        }
        
        if (!validToolCalls.empty()) {
            json assistantMessage = {
                {"role", "assistant"},
                {"content", fullResponse}
            };
            
            if (!validToolCalls.empty()) { 
                assistantMessage["tool_calls"] = validToolCalls; 
            }
            
            messages.push_back(assistantMessage);
            auto toolResults = processToolCalls(validToolCalls);
            
            for (const auto& result : toolResults) {
                messages.push_back(result);
            }
            continue;
        }
        
        break;
    }
    return {fullResponse, messages};
}

void DeepSeekClient::chatStream(const string& userMessage, function<void(const string&)> onChunk, bool useTools) {
    auto [fullResponse, updatedMessages] = chatWithToolsStreaming(userMessage, onChunk, useTools);
    
    if (!fullResponse.empty()) {
        addToChatHistory("assistant", fullResponse);
    }
}

vector<json> DeepSeekClient::getAvailableTools() {
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

json DeepSeekClient::handleShellExec(const json& args) {
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
    
    if (onCommandResult) {
        try {
            (*onCommandResult)(formattedResult);
        } catch (const bad_function_call& e) {
            // Ignore bad callback
        }
    }
    
    return formattedResult;
}

vector<json> DeepSeekClient::processToolCalls(const vector<json>& toolCalls) {
    vector<json> toolResults;
    lastCommandResults.clear();
    pendingCommandResults.clear();
    
    for (const auto& toolCall : toolCalls) {
        if (toolCall.is_null()) continue;
        
        string functionName = toolCall["function"]["name"];
        string argumentsStr = toolCall["function"]["arguments"];
        
        json result;
        
        try {
            json functionArgs = json::parse(argumentsStr);
            
            if (functionName == "shell_exec") {
                result = handleShellExec(functionArgs);
                lastCommandResults.push_back(result);
                pendingCommandResults.push_back(result);
            } else {
                result = {{"error", "Unknown function: " + functionName}, {"status", "error"}};
            }
        } catch (const json::exception& e) {
            result = {{"error", "Error parsing arguments: " + string(e.what())}, {"status", "error"}};
        }
        
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

vector<json> DeepSeekClient::getLastCommandResults() const {
    return lastCommandResults;
}

vector<json> DeepSeekClient::getAndClearPendingCommandResults() {
    vector<json> results = pendingCommandResults;
    pendingCommandResults.clear();
    return results;
}

void DeepSeekClient::clearChatHistory() {
    chatHistory.clear();
}

void DeepSeekClient::addToChatHistory(const string& role, const string& content) {
    chatHistory.push_back({{"role", role}, {"content", content}});
    if (chatHistory.size() > maxChatHistory) {
        chatHistory.erase(chatHistory.begin());
    }
}

size_t DeepSeekClient::StreamingWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    char* data = static_cast<char*>(contents);
    
    StreamData* streamData = static_cast<StreamData*>(userp);
    
    if (!streamData || !streamData->onChunk) { return 0; }
    
    streamData->buffer.append(data, totalSize);
    
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
                            
                            if (delta.contains("content") && !delta["content"].is_null()) {
                                string content = delta["content"];
                                if (!content.empty()) {
                                    streamData->fullResponse += content;
                                    try {
                                        (*streamData->onChunk)(content);
                                    } catch (const bad_function_call& e) {
                                        return 0;
                                    }
                                }
                            }
                            
                            if (delta.contains("tool_calls") && !delta["tool_calls"].empty()) {
                                for (auto& toolCallDelta : delta["tool_calls"]) {
                                    //int index = toolCallDelta.value("index", 0);
                                    size_t index = static_cast<size_t>(toolCallDelta["index"].get<int>());
                                    
                                    if (index >= streamData->toolCalls.size()) {
                                        streamData->toolCalls.resize(index + 1);
                                    }
                                    
                                    auto& toolCall = streamData->toolCalls[index];
                                    
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
                                    
                                    if (toolCallDelta.contains("id")) {
                                        toolCall["id"] = toolCallDelta["id"];
                                    }
                                    
                                    if (toolCallDelta.contains("function") && toolCallDelta["function"].contains("name")) {
                                        toolCall["function"]["name"] = toolCallDelta["function"]["name"];
                                    }
                                    
                                    if (toolCallDelta.contains("function") && toolCallDelta["function"].contains("arguments")) {
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
                    // Ignore parse errors for partial JSON
                }
            }
        }
    }
    
    return totalSize;
}

size_t DeepSeekClient::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

string DeepSeekClient::getCurrentDirectory() const {
    return terminal->getCurrentDir();
}

bool DeepSeekClient::changeDirectory(const string& dir) {
    return terminal->setCurrentDir(dir);
}

int DeepSeekClient::countTextTokens(const string& text) const {
    return TokenCounter::countTokens(text);
}
