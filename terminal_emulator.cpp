#include "terminal_emulator.hpp"
#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <stdexcept>

TerminalEmulator::TerminalEmulator(const string& initialDir) {
    if (initialDir.empty()) {
        char buffer[32768];
        if (getcwd(buffer, sizeof(buffer))) {
            workingDir = buffer;
        } else {
            workingDir = ".";
        }
    } else {
        workingDir = initialDir;
    }
    
    if (!workingDir.empty() && workingDir.back() == '/') {
        workingDir.pop_back();
    }
}

TerminalEmulator::CommandResult TerminalEmulator::executeCommand(const string& command) {
    char originalDir[32768];
    getcwd(originalDir, sizeof(originalDir));
    
    try {
        if (chdir(workingDir.c_str()) != 0) {
            chdir(originalDir);
            return {false, -1, "", "Error changing directory: " + workingDir, command};
        }
        
        string commandStripped = trim(command);
        string tempFile = "/tmp/laia_stderr_" + to_string(time(nullptr)) + ".txt";
        string fullCommand = command + " 2>" + tempFile;
        
        FILE* pipe = popen(fullCommand.c_str(), "r");
        if (!pipe) {
            chdir(originalDir);
            return {false, -1, "", "Error executing command", command};
        }
        
        char buffer[32768];
        string stdoutResult;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            stdoutResult += buffer;
        }
        
        int returnCode = pclose(pipe);
        bool success = (returnCode == 0);
        
        string stderrResult = "";
        FILE* stderrFile = fopen(tempFile.c_str(), "r");
        if (stderrFile) {
            char stderrBuffer[32768];
            while (fgets(stderrBuffer, sizeof(stderrBuffer), stderrFile) != nullptr) {
                stderrResult += stderrBuffer;
            }
            fclose(stderrFile);
            remove(tempFile.c_str());
        }
        
        if (command.find("cd ") != string::npos && success) {
            char currentDir[32768];
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

string TerminalEmulator::getCurrentDir() const { 
    return workingDir; 
}

bool TerminalEmulator::setCurrentDir(const string& newDir) {
    try {
        string targetDir = newDir;
        if (targetDir.find("~") == 0) {
            const char* home = getenv("HOME");
            if (home) {
                targetDir = string(home) + targetDir.substr(1);
            }
        }
        
        if (chdir(targetDir.c_str()) == 0) {
            char buffer[32768];
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

string TerminalEmulator::trim(const string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, last - first + 1);
}
