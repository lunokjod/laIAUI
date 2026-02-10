#ifndef DEEPSEEK_CLIENT_HPP
#define DEEPSEEK_CLIENT_HPP

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <nlohmann/json.hpp>
#include "terminal_emulator.hpp"
#include "token_counter.hpp"

using json = nlohmann::json;
using namespace std;

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
    
    struct StreamData {
        string buffer;
        string fullResponse;
        vector<json> toolCalls;
        bool inToolCall;
        function<void(const string&)>* onChunk;
    };
    
    int countTextTokens(const string& text) const;
    static size_t StreamingWriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    json handleShellExec(const json& args);
    
public:
    DeepSeekClient(const string& apiKey = "", const string& baseUrl = "https://api.deepseek.com");
    ~DeepSeekClient();
    
    void setOnCommandResultCallback(function<void(const json&)> callback);
    pair<string, vector<json>> chatWithToolsStreaming(
        const string& userMessage, 
        function<void(const string&)> onChunk,
        bool useTools = true,
        float temperature = 0.7,
        int maxTokens = 8000,
        int maxIterations = 100);
    
    void chatStream(const string& userMessage, function<void(const string&)> onChunk, bool useTools = false);
    vector<json> getAvailableTools();
    vector<json> processToolCalls(const vector<json>& toolCalls);
    vector<json> getLastCommandResults() const;
    vector<json> getAndClearPendingCommandResults();
    void clearChatHistory();
    void addToChatHistory(const string& role, const string& content);
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    string getCurrentDirectory() const;
    bool changeDirectory(const string& dir);
};

#endif