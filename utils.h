#ifndef MYSHELL_UTILS_H
#define MYSHELL_UTILS_H

#include <string>
#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstdlib>
#include <climits>
#include <fstream>

void substitute_descriptors(std::string &operation, std::vector<std::string> &args);

std::string is_redirect(const std::string &user_input);

std::vector<std::string> tokenize(const std::string &input);

bool endsWith(const std::string &str, const std::string &suffix);

void addToPath();

std::string substituteVariables(const std::string &arg);

int executeCommand(std::vector<std::string> &args, std::string &redirect_operation, bool background);

int parse_msh(const std::string &filename, std::vector<std::string> &commands);


#endif
