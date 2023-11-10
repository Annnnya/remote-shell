//
// Created by Yurii Zinchuk on 10.11.2023.
//

#ifndef MYSHELL_UTILS_H
#define MYSHELL_UTILS_H

#include <string>
#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/fcntl.h>

void substitute_descriptors(std::string &operation, std::vector<std::string> &args);

#endif //MYSHELL_UTILS_H
