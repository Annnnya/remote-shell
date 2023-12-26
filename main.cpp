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
#include <openssl/ssl.h>
#include <openssl/err.h>

int lastExitCode;

void handleClient(SSL* ssl, int clientSocket, int logFileDescriptor, sockaddr_in clientAddress) {
    //to implement client logic here
    char buffer[1024];
    int bytesRead;

    char cwd[PATH_MAX];

    std::string input;
    lastExitCode = 0;
    std::vector<std::string> commands;
    size_t counter = 0;
    bool script_execution = false;

    dup2(SSL_get_fd(ssl), STDOUT_FILENO);
    dup2(SSL_get_fd(ssl), STDERR_FILENO);

    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        std::string prompt = " " + std::string(cwd) + " $ ";
        std::cout << prompt << std::flush;
    } else {
        std::cerr << "Error getting current working directory" << std::endl;
    }

    while ((bytesRead = read(clientSocket, buffer, sizeof(buffer))) > 0) {

        buffer[bytesRead] = '\0';
        input = buffer;
        while (!input.empty() && std::isspace(input.back())) {
            input.pop_back();
        }
        SafeWriteLog(input, getSocketAddressString(clientAddress), logFileDescriptor);

        // main executing code here
        if (!input.empty()) {
            add_history(input.c_str());
            if (containsPipeline(input)) {
                executePipe(input, commands, script_execution);
            } else {
                parseAndExecuteInput(input, commands, script_execution);
            }
        }

        if (script_execution) {
            while (counter < commands.size()) {
                input = commands[counter];
                counter++;
                add_history(input.c_str());
                if (containsPipeline(input)) {
                    executePipe(input, commands, script_execution);
                } else {
                    parseAndExecuteInput(input, commands, script_execution);
                }
            }
            counter = 0;
            commands.clear();
            script_execution = false;
            input = "";
        }

        if (getcwd(cwd, sizeof(cwd)) != nullptr) {
            std::string prompt = " " + std::string(cwd) + " $ ";
            std::cout << prompt << std::flush;
        } else {
            std::cerr << "Error getting current working directory" << std::endl;
        }
    }
}




SSL_CTX *create_context()
{
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = TLS_server_method();

    ctx = SSL_CTX_new(method);
    if (!ctx) {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    return ctx;
}

void configure_context(SSL_CTX *ctx)
{
    /* Set the key and cert */
    if (SSL_CTX_use_certificate_file(ctx, "../tsl_keys/cert.pem", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, "../tsl_keys/key.pem", SSL_FILETYPE_PEM) <= 0 ) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
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

        int logFileDescriptor = open("logs.txt", O_WRONLY | O_APPEND | O_CREAT, 0644);
        if (logFileDescriptor == -1) {
            std::cerr << "Error opening file\n";
            close(serverSocket);
            return -1;
        }

        int sock;
        SSL_CTX *ctx;

        /* Ignore broken pipe signals */
        signal(SIGPIPE, SIG_IGN);

        ctx = create_context();

        configure_context(ctx);

        while (true) {
            //handling clients
            sockaddr_in clientAddress;
            socklen_t clientAddressLen = sizeof(clientAddress);
            SSL *ssl;

            int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressLen);
            if (clientSocket == -1) {
                std::cerr << "Error accepting connection\n";
                continue;
            }

            std::string message = "Connection accepted from " + getSocketAddressString(clientAddress);
            std::cout << message << std::endl;
            SafeWriteLog(message, "Server", logFileDescriptor);
          
            pid_t pid = fork();

            if (pid == 0) {
                // Child process - continues working with client
                close(serverSocket);
                int original_stdout = dup(STDOUT_FILENO);
                int original_stderr = dup(STDERR_FILENO);

                ssl = SSL_new(ctx);
                SSL_set_fd(ssl, clientSocket);
                handleClient(ssl, clientSocket, logFileDescriptor, clientAddress);

                SSL_shutdown(ssl);
                SSL_free(ssl);
                close(clientSocket);

                dup2(original_stdout, STDOUT_FILENO);
                dup2(original_stderr, STDERR_FILENO);

                close(original_stdout);
                close(original_stderr);

                std::cout << "Connection with "<< getSocketAddressString(clientAddress) << " ended." << std::endl;
                SafeWriteLog("Connection ended", getSocketAddressString(clientAddress), logFileDescriptor);
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