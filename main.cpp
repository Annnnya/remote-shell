#include <iostream>
#include <string>
#include <vector>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>
#include <sstream>
#include <climits>
#include <readline/readline.h>
#include <glob.h>
#include <fstream>
#include "internal_functions.h"
#include "utils.h"

int lastExitCode;

int main(int argc, char *argv[]) {
    char cwd[PATH_MAX];

    std::string input;
    lastExitCode = 0;
    std::vector<std::string> commands;
    size_t counter = 0;
    bool script_execution = false;

    addToPath();

    if (argc == 2) {
        int parse_code = parse_msh(argv[1], commands);
        if (parse_code)
            exit(parse_code);
    }

    while (true) {
        if (getcwd(cwd, sizeof(cwd)) != nullptr) {
            if (script_execution) {
                if (counter < commands.size()) {
                    input = commands[counter];
                    counter++;
                } else {
                    counter = 0;
                    commands.clear();
                    script_execution = false;
                    input = "";
                }
            } else if (argc == 2) {
                if (counter < commands.size()) {
                    input = commands[counter];
                    counter++;
                } else exit(0);
            } else {
                std::string prompt = " " + std::string(cwd) + " $ ";
                char *userInput = readline(prompt.c_str());

                if (userInput == nullptr) {
                    break;
                }

                input = userInput;
                free(userInput);
            }

            if (!input.empty()) {
                add_history(input.c_str());
                std::vector<std::string> args;
                for (const std::string &arg: tokenize(input)) {
                    args.push_back(substituteVariables(arg));
                }

                for (size_t i = 0; i < args.size(); ++i) {
                    glob_t glob_result;
                    if (glob(args[i].c_str(), GLOB_NOCHECK, nullptr, &glob_result) == 0) {
                        args.erase(args.begin() + i);

                        for (size_t j = 0; j < glob_result.gl_pathc; ++j) {
                            args.insert(args.begin() + i + j, glob_result.gl_pathv[j]);
                        }

                        i += glob_result.gl_pathc - 1;
                        globfree(&glob_result);
                    }
                }

                bool background = args.back() == "&";
                if (background) args.pop_back();

                std::string redirect_operation = is_redirect(input);
                int default_stdout = dup(STDOUT_FILENO);
                int default_stdin = dup(STDIN_FILENO);
                int default_stderr = dup(STDERR_FILENO);

                if (args[0] == "merrno") {
                    substitute_descriptors(redirect_operation, args);
                    lastExitCode = merrno(args, lastExitCode);
                } else if (args[0] == "mpwd") {
                    substitute_descriptors(redirect_operation, args);
                    lastExitCode = mpwd(args);
                } else if (args[0] == "mcd") {
                    substitute_descriptors(redirect_operation, args);
                    lastExitCode = mcd(args);
                } else if (args[0] == "mexit") {
                    substitute_descriptors(redirect_operation, args);
                    lastExitCode = mexit(args);
                } else if (args[0] == "mecho") {
                    substitute_descriptors(redirect_operation, args);
                    lastExitCode = mecho(args);
                } else if (args[0] == "mexport") {
                    substitute_descriptors(redirect_operation, args);
                    lastExitCode = mexport(args);
                } else if (endsWith(args[0], ".msh")) {
                    substitute_descriptors(redirect_operation, args);
                    if (args.size() == 1) {
                        std::vector<std::string> arg;
                        arg.emplace_back("myshell");
                        arg.push_back(args[0]);
                        lastExitCode = executeCommand(arg, redirect_operation, background);
                    }
                } else if (args[0] == ".") {
                    substitute_descriptors(redirect_operation, args);
                    if (!parse_msh(args[1], commands)) {
                        script_execution = true;
                    }
                } else {
                    lastExitCode = executeCommand(args, redirect_operation, background);
                }

                dup2(default_stdout, STDOUT_FILENO);
                close(default_stdout);
                dup2(default_stdin, STDIN_FILENO);
                close(default_stdin);
                dup2(default_stderr, STDERR_FILENO);
                close(default_stderr);
            }
        } else {
            std::cerr << "Error getting current working directory" << std::endl;
            break;
        }
    }

    return 0;
}