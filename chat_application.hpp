#ifndef CHAT_APPLICATION_HPP
#define CHAT_APPLICATION_HPP

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <nlohmann/json.hpp>
#include "async_task_manager.hpp"
#include "token_counter.hpp"

using json = nlohmann::json;
using namespace std;

class ChatApplication {
public:
    struct ChatMessage {
        string text;
        enum Type { USER, AI, COMMAND_OUTPUT, COMMAND_ERROR, SYSTEM } type;
    };
    
private:
    mutable int currentQueryTokenCount;
    mutable mutex tokenCountMutex;
    mutable int sessionTotalTokens;
    mutable int sessionTotalBytes;
    
    unique_ptr<AsyncTaskManager> asyncTaskManager;
    //char textBuffer[32768];
    std::string textBuffer;
    bool isInitialized;
    bool isProcessingTask;
    string currentProcessingMessage;
    bool requestFocusAfterResponse;
    bool toolsEnabled;
    string streamingResponse;
    bool isStreaming;
    
    mutable mutex messagesMutex;
    mutable mutex pendingResultsMutex;
    vector<json> pendingCommandResults;
    
    struct CommandOutputState {
        bool isExpanded;
        string commandId;
    };
    
    vector<CommandOutputState> commandOutputStates;
    int nextCommandId;
    vector<ChatMessage> chatMessages;
    
    static void renderMarkdownText(const string& text);
    void addPendingCommandResult(const json& result);
    void processPendingCommandResults();
    void showCommandResultImmediately(const json& result);
    
public:
    ChatApplication();
    ~ChatApplication();
    
    bool initialize();
    void update();
    void sendMessage(const string& message);
    void clearChat();
    void changeDirectory(const string& dir);
    void addMessage(const string& text, ChatMessage::Type type);
    
    vector<ChatMessage> getChatMessages() const;
    bool isCommandExpanded(const string& commandId) const;
    void toggleCommandExpanded(const string& commandId);
    
    std::string& getTextBuffer() { return textBuffer; }
    const std::string& getTextBuffer() const { return textBuffer; }

    //size_t getTextBufferSize() const;
    bool getIsInitialized() const;
    bool getIsProcessingTask() const;
    bool getIsStreaming() const;
    bool getToolsEnabled() const;
    void setToolsEnabled(bool enabled);
    string getCurrentDirectory() const;
    bool shouldRequestFocus() const;
    void resetFocusRequest();
    string getAllChatText() const;
    
    int getCurrentQueryTokenCount() const;
    pair<int, int> getSessionStats() const;
    void resetTokenCount();
    void addToSessionStats(int tokens, int bytes);
    
    static void renderTextWithMarkdown(const string& text, bool isStreaming = false);
};

#endif