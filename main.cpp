#include <iostream>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>
#include <sstream>
#include <cstring>
#include <limits.h>

// Function to tokenize a string into a vector of arguments
std::vector<std::string> tokenize(const std::string& input) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(input);
    while (std::getline(tokenStream, token, ' ')) {
        tokens.push_back(token);
    }
    return tokens;
}

// Execute external commands using fork and exec
int executeCommand(const std::vector<std::string>& args) {
    pid_t pid = fork();
    if (pid == -1) {
        std::cerr << "Fork failed." << std::endl;
        return -1;
    } else if (pid == 0) { // Child process
        // Convert vector of strings to array of C-style strings
        std::vector<char*> c_args;
        for (const std::string& arg : args) {
            c_args.push_back(const_cast<char*>(arg.c_str()));
        }
        c_args.push_back(nullptr);

        // Execute the command
        execvp(c_args[0], c_args.data());

        // If execvp fails, print an error message
        std::cerr << "Command not found: " << args[0] << std::endl;
        exit(EXIT_FAILURE);
    } else { // Parent process
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }
        return -1;
    }
}

int main() {
    std::string input;
    std::string prompt = " $ "; // Set a default prompt without the path

    while (true) {
        // Get the current working directory
        char cwd[PATH_MAX]; // PATH_MAX is a system-defined constant
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            // Display the prompt with the current working directory
            std::cout << cwd << prompt;
        } else {
            // Error handling if getcwd fails
            std::cerr << "Error getting current working directory" << std::endl;
            break;
        }

        // Read a line of input from the user
        std::getline(std::cin, input);

        // Tokenize the input into arguments
        std::vector<std::string> args = tokenize(input);

        if (args.empty()) {
            continue; // Empty input, prompt again
        }

        // Check for built-in commands
        if (args[0] == "exit") {
            break; // Exit the shell
        } else if (args[0] == "cd" && args.size() > 1) {
            if (chdir(args[1].c_str()) != 0) {
                std::cerr << "Error changing directory" << std::endl;
            }
        } else {
            // Execute external commands
            int status = executeCommand(args);
            if (status == -1) {
                std::cerr << "Command failed." << std::endl;
            }
        }
    }

    return 0;
}
