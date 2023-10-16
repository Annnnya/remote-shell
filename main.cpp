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
#include <glob.h>
#include "internal_functions.h"

int lastExitCode;

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
        lastExitCode = waitpid(pid, &lastExitCode, 0);
        if (WIFEXITED(lastExitCode)) {
            return WEXITSTATUS(lastExitCode);
        }
        return -1;
    }
}

int main() {
    std::string input;
    lastExitCode = 0;

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

                for (size_t i = 0; i < args.size(); ++i) {
                    glob_t glob_result;
                    if (glob(args[i].c_str(), GLOB_NOCHECK, NULL, &glob_result) == 0) {
                        // Erase the original wildcard argument
                        args.erase(args.begin() + i);
                        
                        // Iterate over the matched paths and insert them into args
                        for (size_t j = 0; j < glob_result.gl_pathc; ++j) {
                            args.insert(args.begin() + i + j, glob_result.gl_pathv[j]);
                        }

                        i += glob_result.gl_pathc - 1;  // Adjust the loop index
                        globfree(&glob_result);
                    }
                }


                if (args[0] == "merrno") {
                    lastExitCode = merrno(args, lastExitCode);
                } else if (args[0] == "mpwd") {
                    lastExitCode = mpwd(args);
                } else if (args[0] == "mcd") {
                    lastExitCode = mcd(args);
                } else if (args[0] == "mexit") {
                    lastExitCode = mexit(args);
                } else if (args[0] == "mecho") {
                    lastExitCode = mecho(args);
                } else {
                    lastExitCode = executeCommand(args);
                }
            }
        } else {
            std::cerr << "Error getting current working directory" << std::endl;
            break;
        }
    }

    return 0;
}