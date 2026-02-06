# laIAUI - AI Assistant GUI with Terminal Execution

A modern GUI application written in C++ that combines an AI assistant with real-time terminal command execution capabilities. 

## Main Features

### 🤖 **Integrated AI Assistant**
- Connection to DeepSeek API with streaming support
- Real-time response streaming with chunk-by-chunk display
- Persistent conversation history (up to 1000 messages)
- Catalan language by default (configurable via system prompt)
- Automatic reading of `prompt.md` files for custom instructions

### 💻 **Terminal Command Execution**
- Safe execution of GNU/Linux terminal commands via `shell_exec` tool
- Real-time stdout/stderr capture and display
- Collapsible command results with detailed information
- Working directory management and navigation
- Command explanations provided by AI

### 🎨 **Modern Graphical Interface**
- Different colors for message types (user, AI, system, commands)
- Auto-scroll and automatic focus management
- Context menu to copy entire chat
- Font support (regular, monospace, bold, italic)

### ⚡ **Real-time Operation**
- AI response streaming with visual cursor indicator
- Asynchronous command execution via worker threads
- Multiple simultaneous task management
- Visual status indicator (Online/Thinking/Offline)
- Non-blocking UI during API calls


## Code Architecture

### Main Classes

1. **`TerminalEmulator`**
   - Safe execution of terminal commands with proper directory handling
   - stdout/stderr capture using temporary files
   - Working directory management with `cd` command support
   - Command result structure with success status and return codes

2. **`DeepSeekClient`**
   - Streaming client for DeepSeek API with tool call support
   - Real-time chunk processing with SSE (Server-Sent Events)
   - Tool call handling for `shell_exec` function
   - Conversation history management with configurable limits

3. **`AsyncTaskManager`**
   - Asynchronous task management with thread-safe queues
   - Streaming worker thread for non-blocking API calls
   - Callback system for real-time updates
   - Pending command result management

4. **`ChatApplication`**
   - Main application logic and GUI state management
   - Message processing with mutex protection
   - Command result display with collapsible sections

### GUI Components

- **Chat Area**: Displays conversation with color-coded messages and markdown-like rendering
- **Text Input**: Multiline input with tab support and adjustable height
- **Command Results**: Collapsible display with detailed information (stdout, stderr, return codes)
- **Status Indicator**: Real-time connection status with blinking "Thinking" animation
- **Tools Toggle**: Enable/disable terminal command execution

## System Requirements

### Dependencies
- **CMake** (>= 3.15)
- **GLFW3** (>= 3.3)
- **OpenGL** (>= 3.3) for GLFW backend
- **cURL** (for HTTP connections to DeepSeek API)
- **nlohmann/json** (for JSON processing)
- **ImGui** (included in the project)
- **Threads** (for asynchronous operations)

### Environment Variables
```bash
export DEEPSEEK_API_KEY="your_deepseek_api_key_here"
```

### Compilation

```bash
# Clone and build
git clone https://github.com/your-username/laIAUI.git
cd laIAUI

# Create build directory
mkdir build && cd build

# Configure (GLFW backend)
cmake ..

# Compile
make -j$(nproc)

# Run
./laIAUI
```

### Platform-Specific Notes

#### Linux
- Requires X11 development libraries for GLFW backend
- Fonts are loaded from `/usr/share/fonts/truetype/dejavu/`
- Uses `popen()` for command execution

## Application Usage

### Starting a Conversation
1. Launch the application (ensure `DEEPSEEK_API_KEY` is set)
2. Type your message in the bottom text area
3. Press Enter or click "Send"
4. Watch real-time streaming responses from the AI

### Command Execution
When "Tools" is enabled, the AI can execute terminal commands automatically. Results appear as collapsible sections showing:

- **Command Explanation**: Why the command was executed
- **Standard Output (stdout)**: Gray text display
- **Error Output (stderr)**: Red text display for errors
- **Current Directory**: Path where command was executed
- **Status & Return Code**: Success/error status with exit code

### Custom Prompts
Create a `prompt.md` file in your working directory to provide additional context to the AI. The application automatically reads and appends this content to the system prompt.

### Keyboard Shortcuts
- **Enter** (in multiline input): Send message
- **Ctrl+N**: Clear chat history
- **Ctrl+Q**: Exit application
- **Right-click in chat area**: Copy entire chat to clipboard

### UI Controls
- **Tools Checkbox**: Toggle terminal command execution capability
- **Current Directory**: Displayed next to tools checkbox
- **Send Button**: Alternative to pressing Enter


### Connection Issues
- Clear error messages when API key is missing
- "Offline" status indicator in menu bar
- Suggestions to check environment variables

### Command Execution Errors
- stderr displayed in red with clear error messages
- Return codes shown for debugging
- Directory change failures handled gracefully

### Streaming Issues
- Partial response display if streaming fails
- Timeout handling for long-running operations
- Memory management for streaming buffers

## Security Considerations

### Command Execution Safety
- Commands execute with current user permissions
- Working directory isolation
- No elevated privileges granted
- stdout/stderr captured separately

### API Key Security
- API key read from environment variable (not hardcoded)
- No local storage of sensitive credentials
- Secure HTTP connections to API endpoint

### Memory Safety
- Smart pointers (`unique_ptr`, `shared_ptr`) for resource management
- RAII pattern for CURL handles and threads
- Proper cleanup in destructors
- Thread-safe data structures with mutex protection

## Performance Features

### Asynchronous Operations
- Non-blocking UI during API calls
- Separate thread for streaming operations
- Queue-based task management
- Efficient memory usage for large conversations

### Streaming Optimization
- Chunk-by-chunk response display
- Immediate tool call processing
- Efficient JSON parsing for SSE streams
- Buffer management for partial data

## Customization

### System Prompt
Modify line 350 in `main.cpp` to change the default system prompt:
```cpp
string systemPrompt = "Ets un assistent AI útil que parla català. Pots executar comandes de terminal quan sigui necessari. No usis emoticones.";
```

### API Configuration
Change the DeepSeek API endpoint in the `DeepSeekClient` constructor (line 170):
```cpp
DeepSeekClient(const string& apiKey = "", const string& baseUrl = "https://api.deepseek.com")
```

### Font Configuration
Modify font loading in `main.cpp` (lines 680-700) to use different fonts or sizes.

### UI Scaling
The application automatically scales based on system DPI settings. Manual scaling options are available in the commented code sections.

## Troubleshooting

### Common Issues

1. **No API Key Error**
   ```
   Error: API key not found. Define DEEPSEEK_API_KEY environment variable
   ```
   Solution: Set the environment variable before running:
   ```bash
   export DEEPSEEK_API_KEY="your_key"
   ./laIAUI
   ```

2. **Missing Dependencies**
   ```
   CMake Error: Could not find package 'glfw3'
   ```
   Solution: Install required packages:
   ```bash
   # Ubuntu/Debian
   sudo apt install libglfw3-dev libcurl4-openssl-dev

   # Fedora
   sudo dnf install glfw-devel libcurl-devel
   ```

## Acknowledgments

- **ImGui**: For the excellent immediate mode GUI library
- **DeepSeek**: For providing accessible AI API services
- **GLFW**: For cross-platform window management
- **nlohmann/json**: For easy JSON handling in C++
- **cURL**: For reliable HTTP client functionality

## License

This project is open source. See LICENSE file for details.

## Contact

For questions, issues, or support:
- Open an issue on the GitHub repository
- Contact the maintainer through project channels

---

**Note**: This application requires a valid DeepSeek API key. Always keep your API keys secure and never commit them to version control.