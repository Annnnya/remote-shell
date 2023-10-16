#include "internal_functions.h"
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <string>
#include <getopt.h>
#include <sys/stat.h>
#include <limits.h>


bool hasHelp(const std::vector<std::string>& args){
    for (size_t i = 1; i < args.size(); ++i) {
        if (args[i] == "-h" || args[i] == "--help") {
            return true;
        }
    }
    return false;
}

int merrno(const std::vector<std::string>& args, int lastExitCode) {
    if (args.size() == 2 && hasHelp(args)){
        std::cout << "Displays last exit code\n\
Usage: merrno [-h|--help]" <<std::endl;
        return 0;
    } else if (args.size() == 1){
        std::cout << "Last exit code: " << lastExitCode << std::endl;
        return 0;
    }
    std::cerr << "Wrong argument number" << std::endl;
    return 1;
}

int mpwd(const std::vector<std::string>& args) {
    if (args.size() == 2 && hasHelp(args)){
        std::cout << "Displays current path\n\
Usage: mpwd [-h|--help]" << std::endl;
        return 0;
    } else if (args.size() == 1){
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
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

int mcd(const std::vector<std::string>& args) {
    if (hasHelp(args)){
        std::cout << "Change current path\n\
Usage: mcd <path> [-h|--help]" << std::endl;
        return 0;
    } else if (args.size() == 2){
        struct stat path_stat;
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


int mexit(const std::vector<std::string>& args) {
    if (args.size() == 2 && hasHelp(args)){
        std::cout << "Exith myshell\n\
Usage: mexit [exit code] [-h|--help]" << std::endl;
        return 0;
    } else if (args.size() == 2) {
        try {
            int n = std::stoi(args[1]);
            exit(n); 
        } catch (const std::invalid_argument&) {
            std::cerr << "Exit code argument is not a valid integer" << std::endl;
            return 1;
        } catch (const std::out_of_range&) {
            std::cerr << "Exit code argument is not a valid integer" << std::endl;
            return 1;
        }
    } else if (args.size() == 1){
        exit(0);
    }
    std::cerr << "Wrong argument number" << std::endl;
    return 1;
}

// int mecho(const std::vector<std::string>& args) {
//     if (hasHelp(args)){
//         std::cout << "Usage: mecho [-h|--help] [text|$<var_name>] [text|$<var_name>] [text|$<var_name>] ..." << std::endl;
//         return 0;
//     } 

// }
