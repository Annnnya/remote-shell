#ifndef INTERNAL_FUNCTIONS_INCLUDE
#define INTERNAL_FUNCTIONS_INCLUDE

#include <string>
#include <vector>


int merrno(const std::vector<std::string>& args, int lastExitCode);

int mpwd(const std::vector<std::string>& args);

int mcd(const std::vector<std::string>& args);

int mexit(const std::vector<std::string>& args);

int mecho(const std::vector<std::string>& args);

#endif