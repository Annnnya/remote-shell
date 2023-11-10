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

std::string is_redirect(const std::string &user_input) {
    if (user_input.find(" < ") != std::string::npos) {
        return "<";
    } else if (user_input.find(" 2>&1") != std::string::npos && user_input.find(" > ") != std::string::npos) {
        return "2>&1";
    } else if (user_input.find(" > ") != std::string::npos) {
        return ">";
    } else if (user_input.find(" &> ") != std::string::npos) {
        return "&>";
    } else {
        return "";
    }
}

std::vector<std::string> tokenize(const std::string &input) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(input);
    bool inDoubleQuotes = false;
    while (std::getline(tokenStream, token, ' ')) {
        if (token[0] == '#' && !inDoubleQuotes)
            break;
        if (!token.empty() && token[0] == '\"' && token[token.length() - 1] == '\"' && token.length() != 1) {
            tokens.push_back(token.substr(1, token.length() - 2));
        } else {
            if (inDoubleQuotes) {
                if (token[token.length() - 1] == '\"') {
                    tokens.back() += " " + token.substr(0, token.length() - 1);
                    inDoubleQuotes = false;
                } else {
                    tokens.back() += " " + token;
                }
            } else {
                if (token[0] == '\"') {
                    tokens.push_back(token.substr(1, token.length()));
                    inDoubleQuotes = true;
                } else {
                    tokens.push_back(token);
                }
            }
        }
    }
    return tokens;
}

bool endsWith(const std::string &str, const std::string &suffix) {
    if (str.length() < suffix.length()) {
        return false;
    }

    size_t pos = str.length() - suffix.length();
    return str.compare(pos, suffix.length(), suffix) == 0;
}

void addToPath() {
    char *programPath = getenv("PATH");

    char programDirectory[PATH_MAX];
    if (getcwd(programDirectory, sizeof(programDirectory)) != nullptr) {

        if (programPath != nullptr) {
            programDirectory[PATH_MAX - 1] = '\0';
            std::string newProgramPath = programDirectory;
            newProgramPath += ":";
            newProgramPath += programPath;
            setenv("PATH", newProgramPath.c_str(), 1);
        } else {
            std::cout << "Error getting PATH environment variable." << std::endl;
        }
    } else {
        std::cout << "Error getting PATH environment variable." << std::endl;
    }
}


std::string substituteVariables(const std::string &arg) {
    std::string result;
    size_t pos = 0;
    while (pos < arg.length()) {
        if (arg[pos] == '$') {
            if (pos + 1 < arg.length() && isalnum(arg[pos + 1])) {
                size_t end = pos + 1;
                while (end < arg.length() && (isalnum(arg[end]) || arg[end] == '_')) {
                    end++;
                }
                std::string varName = arg.substr(pos + 1, end - pos - 1);
                const char *varValue = getenv(varName.c_str());
                if (varValue != nullptr) {
                    result += varValue;
                }
                pos = end;
            } else {
                result += arg[pos];
                pos++;
            }
        } else {
            result += arg[pos];
            pos++;
        }
    }
    return result;
}

int executeCommand(std::vector<std::string> &args, std::string &redirect_operation, bool background) {
    pid_t pid = fork();
    if (pid == -1) {
        std::cerr << "Fork failed." << std::endl;
        return -1;
    } else if (pid == 0) {
        substitute_descriptors(redirect_operation, args);
        if (background && redirect_operation.empty()) {
            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            close(STDERR_FILENO);
        }
        std::vector<std::string> substitutedArgs;
        substitutedArgs.reserve(args.size());
        for (const std::string &arg: args) {
            substitutedArgs.push_back(substituteVariables(arg));
        }
        std::vector<char *> c_args;
        c_args.reserve(substitutedArgs.size());
        for (const std::string &arg: substitutedArgs) {
            c_args.push_back(const_cast<char *>(arg.c_str()));
        }
        c_args.push_back(nullptr);

        execvp(c_args[0], c_args.data());

        std::cerr << "Command not found: " << args[0] << std::endl;
        exit(EXIT_FAILURE);
    } else {
        if (!background) {
            int status;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status)) {
                return WEXITSTATUS(status);
            }
            return -1;
        }
        signal(SIGCHLD, SIG_IGN);
    }
    return 0;
}

int parse_msh(const std::string &filename, std::vector<std::string> &commands) {
    std::ifstream file(filename);
    if (file.is_open()) {
        std::cout << "File opened successfully" << std::endl;
        std::string line;

        while (std::getline(file, line)) {
            if (line[0] != '#')
                commands.push_back(line);
        }
        file.close();
        return 0;
    }
    std::cerr << "Error opening file " << filename << std::endl;
    return 1;
}

int main(int argc, char *argv[]) {
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
        char cwd[PATH_MAX];
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