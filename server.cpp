#include <iostream>     // for cout/cerr
#include <arpa/inet.h>  // for ip inet_pton()
#include <netinet/in.h> // for address
#include <sys/select.h> // for io multiplexing (select)
#include <sys/socket.h> // for socket
#include <unistd.h>     // for close()
#include <vector>       // for storing client
#include <string.h>

/*
 consider adding thread if handling multiple client
 simultaneously  for sending and reciving data at the same time

/*
 structure to encapsulate data of client this make easy to
 passing the argument to new thread;
*/


struct singleClient;

struct Group{
    std::string groupName;
    std::vector<singleClient> membersList;
    Group(void)
    {
        groupName = "defaultGroupName";
    }
};

struct singleClient
{
    int clientId;
    int32_t clientfd;            // client file descriptor
    std::string username;
    std::string password;
    bool log_in_status;
    std::vector<struct Group> chatgroupList;
    singleClient(void)
    {
        clientId = -1;
        clientfd = -1;
        username = "defaultUsername";
        password = "defaultPassword";
        log_in_status = false;
    }
};

struct clientDetails
{
    int32_t clientfd;            // client file descriptor
    int32_t serverfd;            // server file descriptor
    std::vector<singleClient> clientList; // for storing all the client fd
    clientDetails(void)
    { // initializing the variable
        this->clientfd = -1;
        this->serverfd = -1;
    }
};

const int port = 8080;
const char ip[] = "127.0.0.1"; // for local host
// const ip[]="0.0.0.0"; // for allowing all incomming connection from internet
const int backlog = 5; // maximum number of connection allowed
const char password[] = "bean";


int main()
{
    auto client = new clientDetails();

    client->serverfd = socket(AF_INET, SOCK_STREAM, 0); // for tcp connection
    // error handling
    if (client->serverfd <= 0)
    {
        std::cerr << "socket creation error\n";
        delete client;
        exit(1);
    }
    else
    {
        std::cout << "socket created\n";
    }
    // setting serverFd to allow multiple connection
    int opt = 1;
    if (setsockopt(client->serverfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof opt) < 0)
    {
        std::cerr << "setSocketopt error\n";
        delete client;
        exit(2);
    }

    // setting the server address
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serverAddr.sin_addr);
    // binding the server address
    if (bind(client->serverfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        std::cerr << "bind error\n";
        delete client;
        exit(3);
    }
    else
    {
        std::cout << "server binded\n";
    }
    // listening to the port
    if (listen(client->serverfd, backlog) < 0)
    {
        std::cerr << "listen error\n";
        delete client;
        exit(4);
    }
    else
    {
        std::cout << "server is listening\n";
    }

    fd_set readfds;
    size_t valread;
    int maxfd;
    int sd = 0;
    int activity;
    while (true)
    {
        std::cout << "waiting for activity\n";
        FD_ZERO(&readfds);
        FD_SET(client->serverfd, &readfds);
        maxfd = client->serverfd;
        // copying the client list to readfds
        // so that we can listen to all the client
        for (auto currentClient : client->clientList)
        {

            int32_t sd = currentClient.clientfd;
            FD_SET(sd, &readfds);
            if (sd > maxfd)
            {
                maxfd = sd;
            }
        }
        //
        if (sd > maxfd)
        {
            maxfd = sd;
        }
        /* using select for listen to multiple client
           select(int nfds, fd_set *restrict readfds, fd_set *restrict writefds,
           fd_set *restrict errorfds, struct timeval *restrict timeout);
        */

        // for more information about select type 'man select' in terminal
        activity = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0)
        {
            std::cerr << "select error\n";
            continue;
        }
        /*
         * if something happen on client->serverfd then it means its
         * new connection request
         */
        if (FD_ISSET(client->serverfd, &readfds))
        {
            client->clientfd = accept(client->serverfd, (struct sockaddr *)NULL, NULL);
            if (client->clientfd < 0)
            {
                std::cerr << "accept error\n";
                continue;
            }
            // adding client to list

            singleClient temp_client;
            temp_client.clientfd = client->clientfd;
            temp_client.log_in_status = false;
            client->clientList.push_back(temp_client);
            std::cout << "new client connected\n";
            std::cout << "new connection, socket fd is " << client->clientfd << ", ip is: "
                      << inet_ntoa(serverAddr.sin_addr) << ", port: " << ntohs(serverAddr.sin_port) << "\n";



            std::string buffer = "Login:\n";
            send(client->clientfd, buffer.c_str(), strlen(buffer.c_str()), MSG_DONTROUTE);
            /*
             * std::thread t1(handleConnection, client);
             * t1.detach();
             *handle the new connection in new thread
             */
        }
        /*
         * else some io operation on some socket
         */

        // for storing the recive message
        char message[1024];
        for (int i = 0; i < client->clientList.size(); ++i)
        {
            memset(message, 0, 1024);
            sd = client->clientList[i].clientfd;
            if (FD_ISSET(sd, &readfds))
            {
                valread = read(sd, message, 1024);

                // check if client disconnected
                if (valread == 0)
                {
                    std::cout << "client disconnected\n";

                    getpeername(sd, (struct sockaddr *)&serverAddr, (socklen_t *)&serverAddr);
                    // getpeername name return the address of the client (sd)

                    std::cout << "host disconnected, ip: " << inet_ntoa(serverAddr.sin_addr) << ", port: " << ntohs(serverAddr.sin_port) << "\n";
                    close(sd);
                    /* remove the client from the list */
                    client->clientList.erase(client->clientList.begin() + i);
                }

                int message_len = strlen(message);

                char temp_message[1024];

                if (message_len > 2){
                    strcpy(temp_message, message);
                    temp_message[message_len-2] = '\0';
                    if (strcmp(temp_message, "exit") == 0)
                    {
                        std::cout << "client disconnected\n";

                        getpeername(sd, (struct sockaddr *)&serverAddr, (socklen_t *)&serverAddr);
                        // getpeername name return the address of the client (sd)

                        std::cout << "host disconnected, ip: " << inet_ntoa(serverAddr.sin_addr) << ", port: " << ntohs(serverAddr.sin_port) << "\n";
                        close(sd);
                        /* remove the client from the list */
                        client->clientList.erase(client->clientList.begin() + i);
                    }
                    else
                    {
                        if (client->clientList[i].log_in_status == false){
                            if (strcmp(temp_message, password) == 0)
                            {
                                client->clientList[i].log_in_status = true;
                                std::string buffer = "Successful login\n";
                                send(client->clientfd, buffer.c_str(), strlen(buffer.c_str()), MSG_DONTROUTE);
                            }
                            else
                            {
                                std::string temp(temp_message);
                                std::string buffer =  temp + " is incorrect. Try again\n";
                                std::cout << "buffer: " << buffer << std::endl;
                                send(client->clientfd, buffer.c_str(), strlen(buffer.c_str()), MSG_DONTROUTE);
                            }
                        }
                        else{
                            std::cout << "message from client: " << temp_message << "\n";
                            std::string temp(temp_message);
                            if (temp.compare(0, 12, "Set Username") == 0)
                            {
                                client->clientList[i].username = temp.substr(13, temp.length());
                                std::cout << "Client username set to: " <<  client->clientList[i].username << std::endl;
                            }
                            /*
                            * handle the message in new thread
                            * so that we can listen to other client
                            * in the main thread
                            * std::thread t1(handleMessage, client, message);
                            * // detach the thread so that it can run independently
                            * t1.detach();
                            */
                        }

                    }
                }
                
            }
        }
    }
    delete client;
    return 0;
}
