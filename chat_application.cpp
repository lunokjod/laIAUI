#include "chat_application.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include "imgui/imgui.h"


ChatApplication::ChatApplication() : 
    currentQueryTokenCount(0),
    sessionTotalTokens(0),
    sessionTotalBytes(0),
    isInitialized(false),
    isProcessingTask(false),
    requestFocusAfterResponse(false),
    toolsEnabled(false),
    isStreaming(false),
    nextCommandId(0) { 
}

ChatApplication::~ChatApplication() { }

bool ChatApplication::initialize() {
    try {
        asyncTaskManager = make_unique<AsyncTaskManager>();
        
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

void ChatApplication::update() {
    processPendingCommandResults();
}

void ChatApplication::sendMessage(const string& message) {
    if (!isInitialized || message.empty() || isProcessingTask) return;
    
    lock_guard<mutex> lock(tokenCountMutex);
    currentQueryTokenCount = TokenCounter::countTokens(message);
    sessionTotalTokens += TokenCounter::countTokens(message);
    sessionTotalBytes += message.length();

    addMessage("Tu: " + message, ChatMessage::USER);
    
    streamingResponse = "";
    isStreaming = true;
    addMessage("IA: ", ChatMessage::AI);
    
    currentProcessingMessage = message;
    isProcessingTask = true;
    requestFocusAfterResponse = true;
    
    auto totalResponseTokens = make_shared<int>(0);
    auto totalResponseBytes = make_shared<int>(0);
    int chunkCounter = 0;

    asyncTaskManager->submitStreamingTask(
        message,
        [this, chunkCounter, totalResponseTokens, totalResponseBytes](const string& chunk) mutable {
            chunkCounter++;
            cout << "[CHAT] Chunk " << chunkCounter << " obtained: '" << chunk << "'" << endl;

            int chunkTokens = TokenCounter::countTokens(chunk);
            *totalResponseTokens += chunkTokens;
            *totalResponseBytes += chunk.length();
            {
                lock_guard<mutex> lock(tokenCountMutex);
                currentQueryTokenCount = TokenCounter::countTokens(currentProcessingMessage) + *totalResponseTokens;
            }

            streamingResponse += chunk;
            
            {
                lock_guard<mutex> lock(messagesMutex);
                if (!chatMessages.empty()) {
                    for (int i = chatMessages.size() - 1; i >= 0; i--) {
                        if (chatMessages[i].type == ChatMessage::AI) {
                            chatMessages[i].text = "IA: " + streamingResponse;
                            break;
                        }
                    }
                }
            }
            processPendingCommandResults();
        },
        [this, totalResponseTokens, totalResponseBytes]() {
            isStreaming = false;
            isProcessingTask = false;
            {
                lock_guard<mutex> lock(tokenCountMutex);
                currentQueryTokenCount = TokenCounter::countTokens(currentProcessingMessage) + *totalResponseTokens;
                sessionTotalTokens += *totalResponseTokens;
                sessionTotalBytes += *totalResponseBytes;
            }
            
            processPendingCommandResults();
            currentProcessingMessage.clear();
        },
        toolsEnabled
    );
}

void ChatApplication::clearChat() {
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
    {
        lock_guard<mutex> lock(tokenCountMutex);
        currentQueryTokenCount = 0;
    }
    addMessage("Type something to start...", ChatMessage::SYSTEM);
    
    if (isProcessingTask) {
        isProcessingTask = false;
        isStreaming = false;
        currentProcessingMessage.clear();
    }
    requestFocusAfterResponse = true;
}

void ChatApplication::changeDirectory(const string& dir) {
    if (asyncTaskManager && asyncTaskManager->changeDirectory(dir)) {
        addMessage("Folder changed: " + asyncTaskManager->getCurrentDirectory(), ChatMessage::SYSTEM);
    } else {
        addMessage("Unable to change to: " + dir, ChatMessage::SYSTEM);
    }
}

void ChatApplication::addMessage(const string& text, ChatMessage::Type type) {
    lock_guard<mutex> lock(messagesMutex);
    chatMessages.push_back({text, type});
}

vector<ChatApplication::ChatMessage> ChatApplication::getChatMessages() const {
    lock_guard<mutex> lock(messagesMutex);
    return chatMessages;
}

bool ChatApplication::isCommandExpanded(const string& commandId) const {
    lock_guard<mutex> lock(messagesMutex);
    for (const auto& state : commandOutputStates) {
        if (state.commandId == commandId) {
            return state.isExpanded;
        }
    }
    return true;
}

void ChatApplication::toggleCommandExpanded(const string& commandId) {
    lock_guard<mutex> lock(messagesMutex);
    for (auto& state : commandOutputStates) {
        if (state.commandId == commandId) {
            state.isExpanded = !state.isExpanded;
            break;
        }
    }
}

void ChatApplication::addPendingCommandResult(const json& result) {
    lock_guard<mutex> lock(pendingResultsMutex);
    pendingCommandResults.push_back(result);
}

void ChatApplication::processPendingCommandResults() {
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

void ChatApplication::showCommandResultImmediately(const json& result) {
    lock_guard<mutex> lock(messagesMutex);
    
    if (result.contains("status") && result.contains("command")) {
        string commandId = "cmd_" + to_string(nextCommandId++);
        commandOutputStates.push_back({false, commandId});
        
        ChatMessage cmdMessage;
        cmdMessage.type = ChatMessage::COMMAND_OUTPUT;
        cmdMessage.text = commandId + "|" + result.dump();
        
        if (!chatMessages.empty()) {
            for (int i = chatMessages.size() - 1; i >= 0; i--) {
                if (chatMessages[i].type == ChatMessage::AI) {
                    string aiText = chatMessages[i].text;
                    chatMessages.insert(chatMessages.begin() + i + 1, cmdMessage);
                    
                    ChatMessage newAIMessage;
                    newAIMessage.type = ChatMessage::AI;
                    newAIMessage.text = "IA: ";
                    chatMessages.insert(chatMessages.begin() + i + 2, newAIMessage);
                    
                    return;
                }
            }
        }
        
        chatMessages.push_back(cmdMessage);
    }
}

bool ChatApplication::getIsInitialized() const { 
    return isInitialized; 
}

bool ChatApplication::getIsProcessingTask() const { 
    return isProcessingTask; 
}

bool ChatApplication::getIsStreaming() const { 
    return isStreaming; 
}

bool ChatApplication::getToolsEnabled() const { 
    return toolsEnabled; 
}

void ChatApplication::setToolsEnabled(bool enabled) { 
    toolsEnabled = enabled; 
}

string ChatApplication::getCurrentDirectory() const {
    if (asyncTaskManager) {
        return asyncTaskManager->getCurrentDirectory();
    }
    return "N/A";
}

bool ChatApplication::shouldRequestFocus() const { 
    return requestFocusAfterResponse; 
}

void ChatApplication::resetFocusRequest() { 
    requestFocusAfterResponse = false; 
}

string ChatApplication::getAllChatText() const {
    string allText;
    for (const auto& message : chatMessages) {
        allText += message.text + "\n";
    }
    return allText;
}

int ChatApplication::getCurrentQueryTokenCount() const {
    lock_guard<mutex> lock(tokenCountMutex);
    return currentQueryTokenCount;
}

pair<int, int> ChatApplication::getSessionStats() const {
    lock_guard<mutex> lock(tokenCountMutex);
    return {sessionTotalTokens, sessionTotalBytes};
}

void ChatApplication::resetTokenCount() {
    lock_guard<mutex> lock(tokenCountMutex);
    currentQueryTokenCount = 0;
}

void ChatApplication::addToSessionStats(int tokens, int bytes) {
    lock_guard<mutex> lock(tokenCountMutex);
    sessionTotalTokens += tokens;
    sessionTotalBytes += bytes;
}

void ChatApplication::renderMarkdownText(const string& text) {
    if (text.empty()) return;
    
    vector<string> lines;
    stringstream ss(text);
    string line;
    while (getline(ss, line)) {
        lines.push_back(line);
    }
    
    bool inCodeBlock = false;
    string codeBlockLanguage = "";
    int codeLineNumber = 1;
    static bool haveCodeTitle = false;
    
    for (size_t i = 0; i < lines.size(); i++) {
        string currentLine = lines[i];
        
        if (currentLine.find("```") == 0) {
            if (!inCodeBlock) {
                inCodeBlock = true;
                codeBlockLanguage = currentLine.substr(3);
                codeLineNumber = 1;
                haveCodeTitle = true;
                
                if (!codeBlockLanguage.empty()) {
                    ImGui::BeginGroup();
                    
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.31f, 0.16f, 0.47f, 1.0f));
                    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
                    
                    ImGui::BeginChild(("code_header_" + to_string(i)).c_str(), 
                                    ImVec2(0, ImGui::GetTextLineHeight() * 1.5f), 
                                    false, 
                                    ImGuiWindowFlags_NoScrollbar);
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                    
                    float textWidth = ImGui::CalcTextSize(codeBlockLanguage.c_str()).x;
                    float availableWidth = ImGui::GetContentRegionAvail().x;
                    float rightPadding = 50.0f;
                    float offset = availableWidth - textWidth - rightPadding;
                    
                    if (offset > 0) {
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
                    }
                    
                    ImGui::Text("%s", codeBlockLanguage.c_str());
                    ImGui::SameLine();
                    ImGui::Bullet();

                    ImGui::PopStyleColor();
                    ImGui::EndChild();
                    ImGui::PopStyleVar();
                    ImGui::PopStyleColor();
                    
                    ImGui::EndGroup();
                    ImGui::Spacing();
                }
                continue;
            } else {
                if ( haveCodeTitle ) {
                    ImGui::Spacing();
                    ImGui::Separator();
                    haveCodeTitle=false;
                }
                inCodeBlock = false;
                codeBlockLanguage = "";
                codeLineNumber = 1;
                continue;
            }
        }
        
        if (inCodeBlock) {
            ImGui::BeginGroup();
            
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.2f, 0.8f, 1.0f));
            ImGui::SetWindowFontScale(0.8f);
            ImGui::Text("%4d", codeLineNumber);
            ImGui::SetWindowFontScale(1.0f);
            ImGui::PopStyleColor();
            
            ImGui::SameLine(0, 10.0f);
            
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 10.0f));
            
            ImGui::TextWrapped("%s", currentLine.c_str());
            
            ImGui::PopStyleVar(1);
            ImGui::PopStyleColor(2);
            
            ImGui::EndGroup();
            
            codeLineNumber++;
            if (i < lines.size() - 1) {
                ImGui::Spacing();
            }
            continue;
        }
        
        bool isListItem = false;
        string listItemContent = "";
        
        size_t firstNonSpace = currentLine.find_first_not_of(" \t");
        if (firstNonSpace != string::npos) {
            string trimmedLine = currentLine.substr(firstNonSpace);
            
            if (trimmedLine.length() >= 2 && 
                trimmedLine[0] == '*' && 
                trimmedLine[1] == ' ') {
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
        
        bool isBoldLine = false;
        string boldContent = "";
        
        if (currentLine.length() >= 4 && 
            currentLine.find("**") == 0) {
            size_t endBold = currentLine.find("**", 2);
            if (endBold != string::npos && 
                endBold == currentLine.length() - 2) {
                isBoldLine = true;
                boldContent = currentLine.substr(2, endBold - 2);
            }
        }
        
        bool isTitle = false;
        int titleLevel = 0;
        string titleContent = "";
        
        for (int level = 1; level <= 6; level++) {
            string prefix = string(level, '#') + " ";
            if (currentLine.length() >= prefix.length() && 
                currentLine.find(prefix) == 0) {
                isTitle = true;
                titleLevel = level;
                titleContent = currentLine.substr(prefix.length());
                break;
            }
        }
        
        bool isSeparator = false;
        if (currentLine.length() >= 3) {
            string trimmedLine = currentLine;
            size_t firstNonSpace = trimmedLine.find_first_not_of(" \t");
            size_t lastNonSpace = trimmedLine.find_last_not_of(" \t");
            
            if (firstNonSpace != string::npos && lastNonSpace != string::npos) {
                trimmedLine = trimmedLine.substr(firstNonSpace, lastNonSpace - firstNonSpace + 1);
                
                if (trimmedLine.length() >= 3 && 
                    (trimmedLine.find("---") == 0 || trimmedLine.find("***") == 0)) {
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
        
        if (isTitle) {
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
            ImGui::BeginGroup();
            ImGui::Bullet();
            ImGui::SameLine(0, 8.0f);
            ImGui::TextWrapped("%s", listItemContent.c_str());
            ImGui::EndGroup();
        } else if (isBoldLine) {
            ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
            ImGui::SetWindowFontScale(1.1f);
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", boldContent.c_str());
            ImGui::SetWindowFontScale(1.0f);
            ImGui::PopFont();
        } else if (isTitle) {
            ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
            float verticalPadding = 10.0f - (titleLevel * 1.0f);
            if (verticalPadding < 4.0f) verticalPadding = 4.0f;
            
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, verticalPadding));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 4));
            
            float fontSizeScale = 1.5f - (titleLevel * 0.1f);
            if (fontSizeScale < 1.0f) fontSizeScale = 1.0f;
            
            ImGui::SetWindowFontScale(fontSizeScale);
            ImGui::TextWrapped("%s", titleContent.c_str());
            ImGui::SetWindowFontScale(1.0f);
            
            ImGui::PopStyleVar(2);
            ImGui::PopFont();
        } else if (isSeparator) {
            ImGui::Separator();
        } else {
            ImGui::TextWrapped("%s", currentLine.c_str());
        }
        
        if (i < lines.size() - 1) {
            ImGui::Spacing();
        }
    }
}

void ChatApplication::renderTextWithMarkdown(const string& text, bool isStreaming) {
    if (text.empty()) return;

    string displayText = text;
    bool hasPrefix = false;
    if (text.find("IA: ") == 0) {
        displayText = text.substr(4);
        hasPrefix = true;
    } else if (text.find("Tu: ") == 0) {
        displayText = text.substr(4);
        hasPrefix = true;
    }
    
    if (hasPrefix) {
        string prefix = text.substr(0, 4);
        ImGui::Text("%s", prefix.c_str());
        ImGui::SameLine(0, 0);
    }
    renderMarkdownText(displayText);
    
    if (isStreaming) {
        ImGui::SameLine(0, 0);
        
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        float cursorHeight = ImGui::GetTextLineHeight();
        
        static float blinkTimer = 0.0f;
        blinkTimer += ImGui::GetIO().DeltaTime;
        float blinkCycle = fmod(blinkTimer, 1.0f);
        float alpha = (blinkCycle < 0.5f) ? 1.0f : 0.0f;
        
        if (alpha > 0.0f) {
            drawList->AddRectFilled(
                cursorPos,
                ImVec2(cursorPos.x + 8.0f, cursorPos.y + cursorHeight),
                IM_COL32(255, 255, 255, static_cast<int>(255 * alpha))
            );
        }
        ImGui::Dummy(ImVec2(10.0f, 0.0f));
    }
}
