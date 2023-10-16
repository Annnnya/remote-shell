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
#include <readline/readline.h>
#include <readline/history.h>

std::vector<std::string> tokenize(const std::string& input) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(input);
    while (std::getline(tokenStream, token, ' ')) {
        tokens.push_back(token);
    }
    return tokens;
}

int executeCommand(const std::vector<std::string>& args) {
    pid_t pid = fork();
    if (pid == -1) {
        std::cerr << "Fork failed." << std::endl;
        return -1;
    } else if (pid == 0) {
        std::vector<char*> c_args;
        for (const std::string& arg : args) {
            c_args.push_back(const_cast<char*>(arg.c_str()));
        }
        c_args.push_back(nullptr);

        execvp(c_args[0], c_args.data());

        std::cerr << "Command not found: " << args[0] << std::endl;
        exit(EXIT_FAILURE);
    } else {
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

    while (true) {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            std::string prompt = " " + std::string(cwd) + " $ ";
            char* userInput = readline(prompt.c_str());

            if (userInput == nullptr) {
                break; // Exit on Ctrl+D
            }

            input = userInput;
            free(userInput);

            if (!input.empty()) {
                add_history(input.c_str());
                std::vector<std::string> args = tokenize(input);

                if (args[0] == "exit") {
                    break;
                } else if (args[0] == "cd" && args.size() > 1) {
                    if (chdir(args[1].c_str()) != 0) {
                        std::cerr << "Error changing directory" << std::endl;
                    }
                } else {
                    int status = executeCommand(args);
                    if (status == -1) {
                        std::cerr << "Command failed." << std::endl;
                    }
                }
            }
        } else {
            std::cerr << "Error getting current working directory" << std::endl;
            break;
        }
    }

    return 0;
}