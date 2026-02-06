# laIAUI - AI Assistant GUI with Terminal Execution

A modern GUI application written in C++ that combines an AI assistant with real-time terminal command execution capabilities.

## Main Features

### 🤖 **Integrated AI Assistant**
- Connection to DeepSeek API
- Real-time response streaming
- Persistent conversation history
- Catalan language by default (configurable)

### 💻 **Terminal Command Execution**
- Safe execution of GNU/Linux terminal commands
- Real-time stdout/stderr display
- Collapsible results with full details
- Working directory management

### 🎨 **Modern Graphical Interface**
- Based on ImGui + GLFW + OpenGL 3.3
- Rounded and attractive design
- Different colors for message types
- Auto-scroll and automatic focus
- Context menu to copy entire chat

### ⚡ **Real-time Operation**
- AI response streaming
- Asynchronous command execution
- Multiple simultaneous task management
- Visual status indicator (Online/Thinking/Offline)

## Code Structure

### Main Classes

1. **`TerminalEmulator`**
   - Safe execution of terminal commands
   - Working directory management
   - stdout/stderr capture

2. **`DeepSeekClient`**
   - Client for DeepSeek API
   - Support for streaming with tool calls
   - Conversation history management

3. **`AsyncTaskManager`**
   - Asynchronous task management
   - Message queue for streaming
   - Callbacks for real-time updates

4. **`ChatApplication`**
   - Main application logic
   - GUI state management
   - Message and result processing

### GUI Components

- **Chat Area**: Displays conversation with color-coded messages
- **Text Input**: Multiline input with tab support
- **Command Results**: Collapsible display with detailed information
- **Menu Bar**: Clear chat and exit options
- **Status Indicator**: Connection status visualization

## System Requirements

### Dependencies
- **CMake** (>= 3.15)
- **GLFW3** (for window and event management)
- **OpenGL** (>= 3.3)
- **cURL** (for HTTP connections)
- **nlohmann/json** (for JSON processing)
- **ImGui** (included as submodule)

### Environment Variables
```bash
export DEEPSEEK_API_KEY="your_api_key_here"
```

## Compilation and Execution

### Compilation

With glw:

```bash
# Clone the repository
git clone https://github.com/your-username/laIAUI.git
cd laIAUI

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Compile
make
```

With SDL:

```bash
# Clone the repository
git clone https://github.com/your-username/laIAUI.git
cd laIAUI

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. -B build -DUSE_SDL=ON && cmake ..

# Compile
make
```

### Execution
```bash
# Ensure API key is configured
export DEEPSEEK_API_KEY="your_api_key"

# Run the application
./laIAUI
```

## Application Usage

### Starting a Conversation
1. Launch the application
2. Type your message in the bottom text area
3. Press Enter or click "Send"

### Command Execution
The AI can execute terminal commands automatically when deemed necessary. Results are displayed as collapsible elements that include:
- Command explanation
- Standard output (stdout)
- Errors (stderr)
- Current directory
- Status and return code

### Special Functions
- **Ctrl+N**: Clear chat
- **Ctrl+Q**: Exit application
- **Right-click on chat area**: Copy entire chat
- **"Tools" checkbox**: Enable/disable command execution


## GUI Configuration

### Styles
- Rounded windows (10px)
- Rounded buttons (8px)
- Custom colors for message types:
  - **User**: Cyan blue
  - **AI**: Yellow-green
  - **System**: Light green
  - **Commands**: Gray with expandable details

### Responsive Layout
- Automatic resizing
- Adjustable chat area
- Multiline text input
- Adaptive buttons

## Error Management

### Connection Errors
- Clear message when API key is missing
- "Offline" indicator in menu bar
- Suggestion to configure environment variable

### Execution Errors
- stderr display in red
- Visible return code
- Clearly indicated failure status

## Security

### Command Execution
- Execution in current working directory
- Separate stdout/stderr capture
- Permission limitation (runs as current user)

### Memory Management
- Use of smart pointers (unique_ptr)
- Proper resource cleanup
- Exception handling

## Customization

### System Prompt Modification
Edit line 350 in `main.cpp`:
```cpp
string systemPrompt = "Ets un assistent AI útil que parla català. Pots executar comandes de terminal quan sigui necessari.";
```

### Custom Prompt File
You can create a `prompt.md` file in the current directory with additional instructions. The application will automatically read and append it to the system prompt.

### API Change
Modify the `DeepSeekClient` constructor (line 170) to use a different endpoint.

## Contributions

Contributions are welcome! Please:

1. Fork the project
2. Create a branch for your feature
3. Commit your changes
4. Push to the branch
5. Open a Pull Request

## Acknowledgments

- **ImGui**: For the fantastic immediate mode GUI library
- **DeepSeek**: For the accessible AI API
- **GLFW**: For cross-platform window management
- **All contributors**: For making this project possible

## Contact

For questions or support:
- Open an issue on the project GitHub
- Contact the main maintainer

---

**Note**: This application requires a valid DeepSeek API key to function. Make sure to set the `DEEPSEEK_API_KEY` environment variable before running the application.
EOF 2>/tmp/laia_stderr_1770306749.txt
