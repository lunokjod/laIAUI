#ifndef TERMINAL_EMULATOR_HPP
#define TERMINAL_EMULATOR_HPP

#include <string>

using namespace std;

class TerminalEmulator {
private:
    string workingDir;
    
    static string trim(const string& str);
    
public:
    struct CommandResult {
        bool success;
        int returnCode;
        string stdout;
        string stderr;
        string command;
    };

    TerminalEmulator(const string& initialDir = "");
    CommandResult executeCommand(const string& command);
    string getCurrentDir() const;
    bool setCurrentDir(const string& newDir);
};

#endif