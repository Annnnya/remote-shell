//
// Created by Yurii Zinchuk on 10.11.2023.
//

#include "utils.h"

void substitute_descriptors(std::string& redirect_operation, std::vector<std::string>& args) {
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
