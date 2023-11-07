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
#include <fstream>
#include "internal_functions.h"

int lastExitCode;

std::string is_redirect(std::string user_input) {
    if (user_input.find(" < ") != std::string::npos) {
        return "<";
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
        return false;  // The string is shorter than the suffix, so it can't end with it.
    }

    size_t pos = str.length() - suffix.length();
    return str.compare(pos, suffix.length(), suffix) == 0;
}

void addToPath() {
    char *programPath = getenv("PATH");

    // Retrieve the program's directory dynamically
    char programDirectory[PATH_MAX];
    if (getcwd(programDirectory, sizeof(programDirectory)) != nullptr) {

        // Add the program's directory to the PATH
        if (programPath != nullptr) {
            programDirectory[PATH_MAX - 1] = '\0'; // Ensure the path is null-terminated
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
            // Check if it's a valid variable reference
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
                // If not a valid variable reference, treat '$' as a regular character
                result += arg[pos];
                pos++;
            }
        } else {
            // If not a '$', add the character to the result
            result += arg[pos];
            pos++;
        }
    }
    return result;
}

int executeCommand(const std::vector<std::string> &args) {
    pid_t pid = fork();
    if (pid == -1) {
        std::cerr << "Fork failed." << std::endl;
        return -1;
    } else if (pid == 0) {
        std::vector<std::string> substitutedArgs;
        for (const std::string &arg: args) {
            substitutedArgs.push_back(substituteVariables(arg));
        }
        // Convert vector of strings to array of C-style strings
        std::vector<char *> c_args;
        for (const std::string &arg: substitutedArgs) {
            c_args.push_back(const_cast<char *>(arg.c_str()));
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

int parse_msh(std::string filename, std::vector<std::string> &commands) {
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
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
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

                std::string redirect_operation = is_redirect(input);


                int initial_fd_num;
                int saved_initial_fd;
                if (!redirect_operation.empty()) {
                    if (redirect_operation == ">") {
                        std::string filename = args[args.size() - 1];
                        args.pop_back();
                        args.pop_back();
                        FILE *fd = fopen(filename.c_str(), "wr+");
                        if (fd == nullptr) {
                            std::cerr << "Error opening file " << filename << std::endl;
                        }
                        int fd_num = fileno(fd);
                        initial_fd_num = fileno(stdout);

                        saved_initial_fd = dup(initial_fd_num);
                        dup2(fd_num, initial_fd_num);
                        fclose(fd);
                    } else if (redirect_operation == "<") {
                        std::string filename = args[args.size() - 1];
                        args.pop_back();
                        args.pop_back();

                        std::string filename_contents;
                        std::ifstream file(filename);
                        if (file.is_open()) {
                            std::string line;
                            while (std::getline(file, line)) {
                                filename_contents += line;
                            }
                            file.close();
                        } else {
                            std::cerr << "Error opening file " << filename << std::endl;
                        }
                        args.push_back(filename_contents);
                    } else if (redirect_operation == "&>") {
                        // both stdout and stderr are redirected to the same file
                        std::string filename = args[args.size() - 1];
                        args.pop_back();
                        args.pop_back();
                        FILE *fd = fopen(filename.c_str(), "wr+");
                        if (fd == nullptr) {
                            std::cerr << "Error opening file " << filename << std::endl;
                        }
                        int fd_num = fileno(fd);
                        initial_fd_num = fileno(stdout);

                        saved_initial_fd = dup(initial_fd_num);
                        dup2(fd_num, initial_fd_num);
                        dup2(fd_num, fileno(stderr));
                        fclose(fd);
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
                } else if (args[0] == "mexport") {
                    lastExitCode = mexport(args);
                } else if (endsWith(args[0], ".msh")) {
                    if (args.size() == 1) {
                        std::vector<std::string> arg;
                        arg.emplace_back("myshell");
                        arg.push_back(args[0]);
                        lastExitCode = executeCommand(arg);
                    }
                } else if (args[0] == ".") {
                    if (!parse_msh(args[1], commands)) {
                        script_execution = true;
                    }
                } else {
                    lastExitCode = executeCommand(args);
                }

                if (!redirect_operation.empty()) {
                    dup2(saved_initial_fd, initial_fd_num);
                    close(saved_initial_fd);
                }
            }
        } else {
            std::cerr << "Error getting current working directory" << std::endl;
            break;
        }
    }

    return 0;
}