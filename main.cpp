#include <iostream>
#include <string>
#include <vector>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>
#include <sstream>
#include <climits>
#include <readline/readline.h>
#include <readline/history.h>
#include <fstream>
#include "internal_functions.h"
#include "utils.h"
#include <arpa/inet.h>

int lastExitCode;

void handleClient(int clientSocket) {
    //to implement client logic here
    char buffer[1024];
    int bytesRead;

    char cwd[PATH_MAX];

    std::string input;
    lastExitCode = 0;
    std::vector<std::string> commands;
    size_t counter = 0;
    bool script_execution = false;

    dup2(clientSocket, STDOUT_FILENO);
    dup2(clientSocket, STDERR_FILENO);

    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        std::string prompt = " " + std::string(cwd) + " $ ";
        std::cout << prompt << std::flush;
    } else {
        std::cerr << "Error getting current working directory" << std::endl;
    }

    while ((bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
        buffer[bytesRead] = '\0';

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
        } else {
            input = buffer;
            while (!input.empty() && std::isspace(input.back())) {
                input.pop_back();
            }
        }

    // main executing code here
    if (!input.empty()) {
        add_history(input.c_str());
        if (containsPipeline(input)) {
            executePipe(input, commands, script_execution);
        } else {
            parseAndExecuteInput(input, commands, script_execution);
        }
    } else {
            std::cerr << "Error getting current working directory" << std::endl;
            break;
        }

    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        std::string prompt = " " + std::string(cwd) + " $ ";
        std::cout << prompt << std::flush;
    } else {
        std::cerr << "Error getting current working directory" << std::endl;
    }

    }

    close(clientSocket);
}


int main(int argc, char *argv[]) {

    addToPath();
    std::vector<std::string> commands;

    int port;
    bool isServer = parseArguments(argc, argv, port);

    if (argc == 2 && !isServer) {
        int parse_code = parse_msh(argv[1], commands);
        if (parse_code)
            exit(parse_code);
    } else if (!isServer && argc > 1) {
        std::cerr << "Wrong arguments" << std::endl;
        exit(1);
    }



    //establish connection and accept clients
    if (isServer){

        int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == -1) {
            std::cerr << "Error creating socket\n";
            return -1;
        }

        sockaddr_in serverAddress;
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
        serverAddress.sin_port = htons(port);

        if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
            std::cerr << "Error binding socket\n";
            close(serverSocket);
            return -1;
        }

        if (listen(serverSocket, 10) == -1) {
            std::cerr << "Error listening socket\n";
            close(serverSocket);
            return -1;
        }
        std::cout << "Server listening on port " << port << std::endl;


        while (true) {
            //handling clients
            sockaddr_in clientAddress;
            socklen_t clientAddressLen = sizeof(clientAddress);

            int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressLen);
            if (clientSocket == -1) {
                std::cerr << "Error accepting connection\n";
                continue;
            }

            std::cout << "Connection accepted from " << inet_ntoa(clientAddress.sin_addr) << std::endl;

            pid_t pid = fork();

            if (pid == 0) {
                // Child process - continues working with client
                close(serverSocket);;
                handleClient(clientSocket);
                std::cout << "Connection with "<< inet_ntoa(clientAddress.sin_addr) << " ended." << std::endl;
                exit(0);
            } else if (pid > 0) {
                // Parent process - continues listening for new clients
                close(clientSocket);
            } else {
                std::cerr << "Error forking process\n";
                close(clientSocket);
            }
        }

        close(serverSocket);
    }


    char cwd[PATH_MAX];

    std::string input;
    lastExitCode = 0;
    size_t counter = 0;
    bool script_execution = false;

    while (true) {
        // get current working directory and input from user/file
        if (getcwd(cwd, sizeof(cwd)) != nullptr) {
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
            } else if (argc == 2 && !isServer) {
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

        // main executing code here
        if (!input.empty()) {
            add_history(input.c_str());
            if (containsPipeline(input)) {
                executePipe(input, commands, script_execution);
            } else {
                parseAndExecuteInput(input, commands, script_execution);
            }
        } else {
                std::cerr << "Error getting current working directory" << std::endl;
                break;
            }
        }
    }
    return 0;
}