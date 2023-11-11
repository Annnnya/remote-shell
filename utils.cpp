#include "utils.h"


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
