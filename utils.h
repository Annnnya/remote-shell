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
#include <glob.h>
#include <arpa/inet.h>

extern int lastExitCode;

void substitute_descriptors(std::string &operation, std::vector<std::string> &args);

std::string is_redirect(const std::string &user_input);

std::vector<std::string> tokenize(const std::string &input);

bool endsWith(const std::string &str, const std::string &suffix);

void addToPath();

std::string substituteVariables(const std::string &arg);

int executeCommand(std::vector<std::string> &args, std::string &redirect_operation, bool background);

int parse_msh(const std::string &filename, std::vector<std::string> &commands);

bool containsPipeline(const std::string& input);

std::vector<std::string> splitBySubstring(const std::string &input, const std::string &substring);

bool parseArguments(int argc, char* argv[], int& port);

void processInput(std::vector<std::string> &args, std::string &input);

int parseAndExecuteInput(std::string &input, std::vector<std::string> &commands, bool &script_execution);

void executePipe(std::string input, std::vector<std::string> &commands, bool &script_execution);

std::string getCurrentTimeFormatted();

void SafeWriteLog(const std::string& message, std::string who, int logFileDescriptor);

std::string getSocketAddressString(const sockaddr_in& addr);

#endif
