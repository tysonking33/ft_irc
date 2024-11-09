#include <iostream>
#include <cstring>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <algorithm>
#include <thread>
#include <chrono>
#include <mutex>
#include <sstream>
#include <string>

#define PORT 8081
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

std::vector<int> client_sockets; // Shared list of client sockets
std::mutex clients_mutex;        // Mutex for synchronizing access to client_sockets

// Function to set the socket to non-blocking mode
int setNonBlocking(int sockfd)
{
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1)
    {
        perror("fcntl");
        return -1;
    }
    return fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

// Function to print the current status of the server
void printServerStatus()
{
    std::lock_guard<std::mutex> lock(clients_mutex); // Lock for thread safety
    std::cout << "Current connected clients: " << client_sockets.size() << std::endl;
    for (size_t i = 0; i < client_sockets.size(); ++i)
    {
        std::cout << "Client " << i << ": socket fd = " << client_sockets[i] << std::endl;
    }
}

// Function to broadcast a message to all clients
void broadcastMessage(const std::string &message)
{
    std::lock_guard<std::mutex> lock(clients_mutex); // Lock for thread safety
    for (int client_sock : client_sockets)
    {
        send(client_sock, message.c_str(), message.length(), 0);
    }
}

// Function to send a message to a specific client, client_sock is the socket fd
void sendMessageToClient(int client_sock, const std::string &message)
{
    ssize_t bytesSent = send(client_sock, message.c_str(), message.length(), 0);
    if (bytesSent == -1)
    {
        perror("Send failed");
    }
    else
    {
        std::cout << "Sent " << bytesSent << " bytes to client " << client_sock << ": " << message << std::endl;
    }
    std::string messsage1 = "++++++++++";
    send(0, messsage1.c_str(), messsage1.length(), 0);
}


// Function to handle user input
void userInputHandler()
{
    while (true)
    {
        std::cout << "Enter command\n";
        std::cout << "      1. status to see clients\n";
        std::cout << "      2. send [client_number] [message] to send message\n";
        std::cout << "      3. exit to quit): \n";
        std::string command;
        std::getline(std::cin, command);

        if (command == "exit")
        {
            break; // Exit the input handler
        }
        else if (command == "status")
        {
            printServerStatus(); // Print current server status
        }
        else if (command.substr(0, 4) == "send")
        {
            std::istringstream iss(command);
            std::string firstWord, secondWord, message;
            iss >> firstWord >> secondWord; // Read "send" and the client index

            // Get the remaining message
            std::getline(iss, message);
            if (!message.empty() && message[0] == ' ')
            {
                message.erase(0, 1); // Remove leading space
            }

            int client_index = stoi(secondWord);
            if (client_index < 0 || client_index >= client_sockets.size())
            {
                std::cout << "Invalid client number." << std::endl;
                continue;
            }

            int client_sock = client_sockets[client_index]; // Correctly get the socket fd
            sendMessageToClient(client_sock, message);
            std::cout << "Server sent to client " << client_sock << ": " << message << std::endl;
        }
        else
        {
            std::cout << "Unknown command." << std::endl;
        }
    }
}

int main()
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket to non-blocking
    if (setNonBlocking(server_fd) == -1)
    {
        perror("Failed to set non-blocking");
        exit(EXIT_FAILURE);
    }

    // Setup server address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Start listening
    if (listen(server_fd, MAX_CLIENTS) < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server is listening on port " << PORT << "..." << std::endl;

    // Start a thread to handle user input
    std::thread input_thread(userInputHandler);

    while (true)
    {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        int max_sd = server_fd;

        // Add child sockets to set
        {
            std::lock_guard<std::mutex> lock(clients_mutex); // Lock for thread safety
            for (int i : client_sockets)
            {
                if (i > 0)
                {
                    FD_SET(i, &readfds);
                }
                if (i > max_sd)
                {
                    max_sd = i;
                }
            }
        }

        // Wait for activity on sockets
        int activity = select(max_sd + 1, &readfds, nullptr, nullptr, nullptr);

        // Check for new connections
        if (FD_ISSET(server_fd, &readfds))
        {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
            {
                perror("Accept failed");
                exit(EXIT_FAILURE);
            }
            std::cout << "New connection accepted: socket fd is " << new_socket << std::endl;

            {
                std::lock_guard<std::mutex> lock(clients_mutex); // Lock for thread safety
                client_sockets.push_back(new_socket);
            }
        }

        // Check for incoming messages from clients
        for (size_t i = 0; i < client_sockets.size(); ++i)
        {
            int client_sock = client_sockets[i];
            if (FD_ISSET(client_sock, &readfds))
            {
                int valread = read(client_sock, buffer, BUFFER_SIZE);
                if (valread == 0)
                {
                    // Client disconnected
                    std::cout << "Client disconnected: socket fd is " << client_sock << std::endl;
                    close(client_sock);
                    {
                        std::lock_guard<std::mutex> lock(clients_mutex); // Lock for thread safety
                        client_sockets.erase(client_sockets.begin() + i);
                    }
                    --i;
                }
                else
                {
                    buffer[valread] = '\0';
                    std::cout << "Message from client: " << buffer << std::endl;

                    // Echo the message back to the client
                    //send(client_sock, buffer, strlen(buffer), 0);
                }
            }
        }
    }

    input_thread.join(); // Wait for the input thread to finish
    close(server_fd);
    return 0;
}
