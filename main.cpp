#include <iostream>
#include <string>
#include <vector>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>
#include <sstream>
#include <climits>
#include <readline/readline.h>
#include <readline/history.h>
#include <glob.h>
#include <fstream>
#include "internal_functions.h"
#include "utils.h"

int lastExitCode;

void processInput(std::vector<std::string> &args, std::string &input){
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
}

int parseAndExecuteInput(std::string &input, std::vector<std::string> &commands, bool &script_execution){
    std::vector<std::string> args;
    processInput(args, input);

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

    return 0;
}


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

        // main executiong code here
        if (!input.empty()) {
            add_history(input.c_str());
            if (containsPipeline(input)) {
                std::vector<std::string> pipeline_commands = splitBySubstring(input, " | ");
                int num_pipes = pipeline_commands.size() - 1;

                std::vector<std::vector<int>> pipes(num_pipes, std::vector<int>(2, 0));
                for (int i = 0; i < num_pipes; ++i) {
                    if (pipe(pipes[i].data()) == -1) {
                        std::cerr << "Error creating pipe." << std::endl;
                        exit(EXIT_FAILURE);
                    }
                }

                for (int i = 0; i <= num_pipes; ++i) {
                    std::string command = pipeline_commands[i];
                    std::vector<std::string> args;
                    processInput(args, command);

                    pid_t pid = fork();
                    if (pid == -1) {
                        std::cerr << "Fork failed." << std::endl;
                        exit(EXIT_FAILURE);
                    } else if (pid == 0) {
                        if (i != 0) {
                            dup2(pipes[i - 1][0], STDIN_FILENO);
                            close(pipes[i - 1][0]);
                            close(pipes[i - 1][1]);
                        }
                        if (i != num_pipes) {
                            dup2(pipes[i][1], STDOUT_FILENO);
                            close(pipes[i][0]);
                            close(pipes[i][1]);
                        }
                        for (int j = 0; j < num_pipes; ++j) {
                            close(pipes[j][0]);
                            close(pipes[j][1]);
                        }
                        parseAndExecuteInput(command, commands, script_execution);
                        exit(EXIT_SUCCESS);
                    }
                }

                for (int i = 0; i < num_pipes; ++i) {
                    close(pipes[i][0]);
                    close(pipes[i][1]);
                }

                for (int i = 0; i <= num_pipes; ++i) {
                    int status;
                    wait(&status);
                    if (WIFEXITED(status)) {
                        lastExitCode = WEXITSTATUS(status);
                    }
                }

            } else {
                parseAndExecuteInput(input, commands, script_execution);
            }
        } else {
                std::cerr << "Error getting current working directory" << std::endl;
                break;
            }
        }
    }
    return 0;
}