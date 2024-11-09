#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/select.h>
#include <thread>
#include <atomic>

#define PORT 8081
#define BUFFER_SIZE 1024

std::atomic<bool> running(true); // To control the input thread

void inputHandler(int sock) {
    while (running) {
        std::string message;
        std::getline(std::cin, message);

        if (message.empty()) {
            std::cerr << "Cannot send an empty message" << std::endl;
            continue; // Skip sending
        }

        ssize_t bytesSent = send(sock, message.c_str(), message.size(), 0);
        if (bytesSent < 0) {
            std::cerr << "Error sending message: " << strerror(errno) << std::endl;
        }
        std::cout << "Client sends: " << message << "\n";
    }
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return -1;
    }

    // Set the socket to non-blocking
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address / Address not supported" << std::endl;
        return -1;
    }

    // Attempt to connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        if (errno != EINPROGRESS) {
            std::cerr << "Connection failed" << std::endl;
            return -1;
        }
    }

    // Use select to wait for the socket to become writable
    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(sock, &writefds);

    struct timeval timeout;
    timeout.tv_sec = 5; // 5 seconds timeout
    timeout.tv_usec = 0;

    int activity = select(sock + 1, nullptr, &writefds, nullptr, &timeout);
    if (activity < 0) {
        std::cerr << "Select error" << std::endl;
        return -1;
    } else if (activity == 0) {
        std::cerr << "Connection timed out" << std::endl;
        return -1;
    } else if (FD_ISSET(sock, &writefds)) {
        int so_error;
        socklen_t len = sizeof(so_error);
        if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len) < 0) {
            std::cerr << "getsockopt failed" << std::endl;
            return -1;
        }
        if (so_error != 0) {
            std::cerr << "Connection failed: " << strerror(so_error) << std::endl;
            return -1;
        }
    }

    std::cout << "Connected to the server!" << std::endl;

    // Start the input thread
    std::thread input_thread(inputHandler, sock);

    while (running) {
        // Clear the buffer
        memset(buffer, 0, BUFFER_SIZE);

        // Use select to check for data available to read
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);

        // Also check for stdin input
        fd_set readfds_stdin;
        FD_ZERO(&readfds_stdin);
        FD_SET(STDIN_FILENO, &readfds_stdin);

        // Set a timeout of 5 seconds
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;

        // Combine both sets
        fd_set combined_fds = readfds;
        combined_fds = readfds;

        // Wait for either socket or stdin input
        activity = select(sock + 1, &combined_fds, nullptr, nullptr, &timeout);
        if (activity < 0) {
            std::cerr << "Select error" << std::endl;
            break;
        } else if (activity == 0) {
            //std::cerr << "Timeout waiting for server response" << std::endl;
            continue; // continue waiting for server response
        }

        // Check for server responses
        if (FD_ISSET(sock, &combined_fds)) {
            int valread = read(sock, buffer, BUFFER_SIZE);
            if (valread < 0) {
                std::cerr << "Error reading from server" << std::endl;
                break;
            } else if (valread == 0) {
                // Server has closed the connection
                std::cerr << "Server closed the connection" << std::endl;
                running = false; // Stop the input thread
                break;
            }
            buffer[valread] = '\0';
            std::cout << "Message from server: " << buffer << std::endl;
        }

        // Check for user input
        if (FD_ISSET(STDIN_FILENO, &readfds_stdin)) {
            // Handle user input directly in the main thread (optional)
            // This section can be empty as the input is handled in the thread
        }
    }

    running = false; // Signal the input thread to stop
    input_thread.join(); // Wait for the input thread to finish
    close(sock);
    return 0;
}
