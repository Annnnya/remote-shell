#include "internal_functions.h"
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <string>
#include <sys/stat.h>
#include <climits>


bool hasHelp(const std::vector<std::string> &args) {
    for (size_t i = 1; i < args.size(); ++i) {
        if (args[i] == "-h" || args[i] == "--help") {
            return true;
        }
    }
    return false;
}

int merrno(const std::vector<std::string> &args, int lastExitCode) {
    if (args.size() == 2 && hasHelp(args)) {
        std::cout << "Displays last exit code\nUsage: merrno [-h|--help]" << std::endl;
        return 0;
    } else if (args.size() == 1) {
        std::cout << "Last exit code: " << lastExitCode << std::endl;
        return 0;
    }
    std::cerr << "Wrong argument number" << std::endl;
    return 1;
}

int mpwd(const std::vector<std::string> &args) {
    if (args.size() == 2 && hasHelp(args)) {
        std::cout << "Displays current path\nUsage: mpwd [-h|--help]" << std::endl;
        return 0;
    } else if (args.size() == 1) {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != nullptr) {
            std::cout << cwd << std::endl;
            return 0;
        } else {
            std::cerr << "Error getting current working directory" << std::endl;
            return 1;
        }
    }
    std::cerr << "Wrong argument number" << std::endl;
    return 1;
}

int mcd(const std::vector<std::string> &args) {
    if (hasHelp(args)) {
        std::cout << "Change current path\nUsage: mcd <path> [-h|--help]" << std::endl;
        return 0;
    } else if (args.size() == 2) {
        struct stat path_stat{};
        if (stat(args[1].c_str(), &path_stat) == 0) {
            if (S_ISDIR(path_stat.st_mode)) {
                if (chdir(args[1].c_str()) == 0) {
                    return 0;
                }
            }
        }
        std::cerr << "Invalid directory path or is not a directory" << std::endl;
        return 1;
    }
    std::cerr << "Wrong argument number" << std::endl;
    return 1;
}


int mexit(const std::vector<std::string> &args) {
    if (args.size() == 2 && hasHelp(args)) {
        std::cout << "Exit myshell\nUsage: mexit [exit code] [-h|--help]" << std::endl;
        return 0;
    } else if (args.size() == 2) {
        try {
            int n = std::stoi(args[1]);
            exit(n);
        } catch (const std::invalid_argument &) {
            std::cerr << "Exit code argument is not a valid integer" << std::endl;
            return 1;
        } catch (const std::out_of_range &) {
            std::cerr << "Exit code argument is not a valid integer" << std::endl;
            return 1;
        }
    } else if (args.size() == 1) {
        exit(0);
    }
    std::cerr << "Wrong argument number" << std::endl;
    return 1;
}

int mexport(const std::vector<std::string> &args) {
    if (args[0] == "mexport" && args.size() == 2) {
        const std::string &arg = args[1];
        size_t pos = arg.find('=');
        if (pos != std::string::npos && pos > 0 && pos < arg.length() - 1) {
            if (arg.find("$(") == pos + 1) {
                std::string varName = arg.substr(0, pos);
                std::string varValue = arg.substr(pos + 3, arg.length() - pos - 4);
                std::string command = varValue;
                FILE *fp = popen(command.c_str(), "r");
                if (fp == nullptr) {
                    std::cerr << "Error executing command" << std::endl;
                    return 1;
                }
                char buf[1024];
                if (fgets(buf, 1024, fp) != nullptr) {
                    varValue = buf;
                    varValue.erase(varValue.length() - 1);
                    if (setenv(varName.c_str(), varValue.c_str(), 1) != 0) {
                        std::cerr << "Error setting environment variable." << std::endl;
                        return 1;
                    }
                    return 0;
                } else {
                    std::cerr << "Error executing command" << std::endl;
                    return 1;
                }
            } else {
                std::string varName = arg.substr(0, pos);
                std::string varValue = arg.substr(pos + 1);

                if (setenv(varName.c_str(), varValue.c_str(), 1) != 0) {
                    std::cerr << "Error setting environment variable." << std::endl;
                    return 1;
                }
                return 0;
            }
        }
    }
    return 1;
}

int mecho(const std::vector<std::string> &args) {
    if (hasHelp(args)) {
        std::cout << "Usage: mecho [-h|--help] [text|$<var_name>] [text|$<var_name>] [text|$<var_name>] ..."
                  << std::endl;
        return 0;
    } else {
        for (size_t i = 1; i < args.size(); ++i) {
            std::cout << args[i] << " ";
        }
        std::cout << std::endl;
        return 0;
    }
}
