#include "utils.h"
#include <string.h>
#include <getopt.h>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <sys/file.h> 
#include "internal_functions.h"


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

bool containsPipeline(const std::string& input) {
    return input.find(" | ") != std::string::npos;
}

std::vector<std::string> splitBySubstring(const std::string &input, const std::string &substring) {
    std::vector<std::string> result;

    size_t startPos = 0;
    size_t foundPos = input.find(substring);

    while (foundPos != std::string::npos) {
        result.push_back(input.substr(startPos, foundPos - startPos));
        startPos = foundPos + substring.length();
        foundPos = input.find(substring, startPos);
    }

    result.push_back(input.substr(startPos));
    return result;
}

void substitute_descriptors(std::string &redirect_operation, std::vector<std::string> &args) {
    if (redirect_operation == ">") {
        int fd = open(args[args.size() - 1].c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd == -1) {
            std::cerr << "Error opening file " << args[args.size() - 1] << std::endl;
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
        args.erase(args.end() - 2, args.end());
    } else if (redirect_operation == "<") {
        int fd = open(args[args.size() - 1].c_str(), O_RDONLY);
        if (fd == -1) {
            std::cerr << "Error opening file " << args[args.size() - 1] << std::endl;
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
        args.erase(args.end() - 2, args.end());
    } else if (redirect_operation == "&>") {
        int fd = open(args[args.size() - 1].c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd == -1) {
            std::cerr << "Error opening file " << args[args.size() - 1] << std::endl;
        }
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        close(fd);
        args.erase(args.end() - 2, args.end());
    } else if (redirect_operation == "2>&1") {
        int fd = open(args[args.size() - 2].c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd == -1) {
            std::cerr << "Error opening file " << args[args.size() - 1] << std::endl;
        }
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        close(fd);
        args.erase(args.end() - 3, args.end());
    }
}



bool parseArguments(int argc, char* argv[], int& port) {
    bool isServer = false;
    port = 8080;
    int option;

    struct option longOptions[] = {
        {"server", no_argument, nullptr, 's'},
        {"port", required_argument, nullptr, 'p'},
        {nullptr, 0, nullptr, 0}
    };

    while ((option = getopt_long(argc, argv, "sp:", longOptions, nullptr)) != -1) {
        switch (option) {
            case 's':
                isServer = true;
                break;
            case 'p':
                port = atoi(optarg);
                break;
            default:
                std::cerr << "Usage: myshell [--server] [--port PORT_NUMBER]\n";
                exit(EXIT_FAILURE);
        }
    }
    return isServer;
}


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


void executePipe(std::string input, std::vector<std::string> &commands, bool &script_execution){
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
}


std::string getCurrentTimeFormatted() {
    auto currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    std::tm* localTime = std::localtime(&currentTime);

    std::ostringstream oss;
    oss << std::put_time(localTime, "%Y-%m-%d %H:%M:%S");

    return oss.str();
}

void SafeWriteLog(const std::string& message, std::string who, int logFileDescriptor) {
    std::string time = getCurrentTimeFormatted();
    std::string logMessage = "[" + time + "] @" + who + " : " + message + "\n";
    if (flock(logFileDescriptor, LOCK_EX) == 0) {
        write(logFileDescriptor, logMessage.c_str(), logMessage.length());
        flock(logFileDescriptor, LOCK_UN);
    } else {
        std::cerr << "Error locking log file." << std::endl;
    }
}

std::string getSocketAddressString(const sockaddr_in& addr) {
    char ipStr[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &(addr.sin_addr), ipStr, INET_ADDRSTRLEN) == nullptr) {
        std::cerr << "Error converting IP address to string.\n";
        return "";
    }

    uint16_t port = ntohs(addr.sin_port);
    return std::string(ipStr) + ":" + std::to_string(port);
}

