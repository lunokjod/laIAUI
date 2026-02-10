#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "GLFW/glfw3.h"
#include "chat_application.hpp"
#include <iostream>
#include <csignal>
#include "imgui/misc/cpp/imgui_stdlib.h"

using namespace std;

// Global atomic for interrupt handling
atomic<bool> g_interrupted(false);
atomic<bool> g_inChatLoop(false);

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    // Initialize GLFW
    if (!glfwInit()) {
        cerr << "ERROR: Unable to initialize glfw" << endl;
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
        cerr << "ERROR: Unable to create glfw window" << endl;
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

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Set aspect
    ImGuiStyle& style = ImGui::GetStyle();
    style.FrameRounding = 8.0f;
    style.FrameBorderSize = 1.0f;
    style.WindowRounding = 10.0f;
    style.PopupRounding = 8.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding = 8.0f;
    style.TabRounding = 8.0f;
    style.ChildRounding = 8.0f;
    
    //ImVec4 bgColor = ImVec4(0.15f, 0.18f, 0.25f, 1.0f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.23f, 0.30f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.28f, 0.35f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.33f, 0.40f, 1.0f);

    // Create chat application
    ChatApplication chatApp;
    bool appInitialized = chatApp.initialize();
    
    static float chatInputHeight = 40.0f;
    static bool setFocusToInput = true;
    
    // Main loop
    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS && chatApp.isInferenceCancellable()) {
            chatApp.cancelCurrentInference();
        }
        glfwPollEvents();
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
            ImGui::Bullet();
            if (appInitialized) {
                if (chatApp.getIsProcessingTask()) {
                    static float blinkTimer = 0.0f;
                    blinkTimer += ImGui::GetIO().DeltaTime;
                    float blinkCycle = fmod(blinkTimer, 1.0f);
                    float alpha = (blinkCycle < 0.5f) ? 1.0f : 0.3f;
                    
                    int tokenCount = chatApp.getCurrentQueryTokenCount();
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, alpha), "Inferencing (%d)...", tokenCount);
                } else {
                    auto [sessionTokens, sessionBytes] = chatApp.getSessionStats();
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Ready (%d tokens, %d chars)", 
                                      sessionTokens, sessionBytes);
                }
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Unable to connect");
            }
            ImGui::EndMenuBar();
        }
        
        // Get window size
        ImVec2 windowSize = ImGui::GetWindowSize();
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

        ImGui::BeginChild("ChatArea", ImVec2(0, availableHeight), false, ImGuiWindowFlags_None);
        
        // Context menu
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
            
            if (message.type == ChatApplication::ChatMessage::COMMAND_OUTPUT && 
                message.text.find("cmd_") == 0) {
                
                size_t separatorPos = message.text.find('|');
                if (separatorPos != string::npos) {
                    string commandId = message.text.substr(0, separatorPos);
                    string jsonData = message.text.substr(separatorPos + 1);
                    
                    try {
                        json result = json::parse(jsonData);
                        if (result.contains("explanation") && !result["explanation"].get<string>().empty()) {
                            string explanation = result["explanation"];
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
                            ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
                            ImGui::TextWrapped("> %s", explanation.c_str());
                            ImGui::PopFont();
                            ImGui::PopStyleColor();
                            ImGui::Spacing();
                        }
                        
                        bool isExpanded = chatApp.isCommandExpanded(commandId);
                        string headerLabel = "cmd: " + result["command"].get<string>();
                        
                        float availableWidth = ImGui::GetContentRegionAvail().x;
                        float headerTextWidth = ImGui::CalcTextSize(headerLabel.c_str()).x;
                        
                        if (headerTextWidth > availableWidth) {
                            string truncatedLabel = headerLabel;
                            for (int len = headerLabel.length() - 1; len > 1; len--) {
                                truncatedLabel = headerLabel.substr(0, len) + "...";
                                float truncatedWidth = ImGui::CalcTextSize(truncatedLabel.c_str()).x;
                                if (truncatedWidth <= availableWidth) {
                                    headerLabel = truncatedLabel;
                                    break;
                                }
                            }
                        }
                        
                        ImGui::PushID(commandId.c_str());
                        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
                        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.4f, 0.4f, 0.4f, 0.7f));
                        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.5f, 0.5f, 0.5f, 0.9f));
                        
                        bool headerOpen = ImGui::CollapsingHeader(headerLabel.c_str(), 
                            isExpanded ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_None);
                        
                        ImGui::PopStyleColor(3);
                        
                        if (headerOpen != isExpanded) {
                            chatApp.toggleCommandExpanded(commandId);
                        }
                        
                        if (headerOpen) {
                            ImGui::Indent();
                            
                            if (result.contains("stdout") && !result["stdout"].get<string>().empty()) {
                                string stdoutText = result["stdout"];
                                if (!stdoutText.empty() && stdoutText.back() == '\n') {
                                    stdoutText.pop_back();
                                }
                                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
                                ImGui::TextWrapped("%s", stdoutText.c_str());
                                ImGui::PopStyleColor();
                            }
                            
                            if (result.contains("stderr") && !result["stderr"].get<string>().empty()) {
                                string stderrText = result["stderr"];
                                if (!stderrText.empty() && stderrText.back() == '\n') {
                                    stderrText.pop_back();
                                }
                                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
                                ImGui::TextWrapped("%s", stderrText.c_str());
                                ImGui::PopStyleColor();
                            }
                            
                            ImGui::Spacing();
                            ImGui::Separator();
                            ImGui::Spacing();
                            
                            if (result.contains("currentDirectory")) {
                                string dir = result["currentDirectory"];
                                ImGui::Text("path: %s", dir.c_str());
                            }

                            if (result.contains("status")) {
                                string status = result["status"];
                                ImVec4 statusColor = (status == "success") ? 
                                    ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : 
                                    ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
                                ImGui::TextColored(statusColor, "Status: %s", status.c_str());
                                
                                ImGui::SameLine();
                            }
                            if (result.contains("returnCode")) {
                                int returnCode = result["returnCode"];
                                ImGui::Text("ret: %d", returnCode);
                            }                            
                            ImGui::Unindent();
                        }
                        
                        ImGui::PopID();
                        
                    } catch (const json::exception& e) {
                        ImGui::TextWrapped("%s", message.text.c_str());
                    }
                    continue;
                }
            }
            
            // Other kind of messages only uses colors
            switch (message.type) {
                case ChatApplication::ChatMessage::USER:
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.8f, 1.0f, 1.0f));
                    break;
                case ChatApplication::ChatMessage::AI:
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.9f, 0.3f, 1.0f));
                    break;
                case ChatApplication::ChatMessage::COMMAND_OUTPUT:
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
                    break;
                case ChatApplication::ChatMessage::COMMAND_ERROR:
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
                    break;
                case ChatApplication::ChatMessage::SYSTEM:
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.8f, 0.5f, 1.0f));
                    break;
            }

            bool isLastMessage = (i == messages.size() - 1);
            bool isAIStreaming = (isLastMessage && 
                                message.type == ChatApplication::ChatMessage::AI &&
                                chatApp.getIsStreaming());
            ChatApplication::renderTextWithMarkdown(message.text, isAIStreaming);
            ImGui::PopStyleColor();
            
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
            &chatApp.getTextBuffer(), // Passar per referència
            ImVec2(textInputWidth, chatInputHeight),
            ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_EnterReturnsTrue
        );
        ImGui::PopItemWidth();
        
        // Send button
        ImGui::SameLine();
        bool sendButtonPressed = ImGui::Button("Send", ImVec2(70, chatInputHeight));

        // Tools checkbox area
        ImGui::Spacing();
        bool toolsEnabled = chatApp.getToolsEnabled();
        if (ImGui::Checkbox("Tools", &toolsEnabled)) {
            chatApp.setToolsEnabled(toolsEnabled);
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
        if ((textEnterPressed || sendButtonPressed) && 
            !chatApp.getTextBuffer().empty() && 
            !chatApp.getIsProcessingTask()) {
            
            string message = chatApp.getTextBuffer();
            chatApp.sendMessage(message);
            chatApp.getTextBuffer().clear();  // Netejar
            setFocusToInput = true;
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
