#include <iostream>     // for cout/cerr
#include <arpa/inet.h>  // for ip inet_pton()
#include <netinet/in.h> // for address
#include <sys/select.h> // for io multiplexing (select)
#include <sys/socket.h> // for socket
#include <unistd.h>     // for close()
#include <vector>       // for storing client
#include <string.h>
#include <memory>
#include <sstream>
#include <algorithm>

/*
 consider adding thread if handling multiple client
 simultaneously  for sending and reciving data at the same time

/*
 structure to encapsulate data of client this make easy to
 passing the argument to new thread;
*/

struct Group; // Forward declaration

struct singleClient
{
    int clientId;
    int32_t clientfd; // Client file descriptor
    std::string username;
    bool log_in_status;
    std::vector<Group *> chatgroupList; // Use raw pointers to Group

    singleClient()
        : clientId(-1), clientfd(-1), username("defaultUsername"), log_in_status(false) {}

    ~singleClient()
    {
        // Destructor doesn't delete chatgroupList elements
        // Ownership is handled by the container or elsewhere
    }
};

struct Group
{
    std::string groupName;
    std::vector<singleClient *> membersList; // Use raw pointers to singleClient
    bool inviteOnly;
    std::string topic;
    singleClient *owner; // Raw pointer to owner
    bool password_on;
    std::string password;
    bool member_limit_on;
    int memberLimit;

    Group()
        : groupName("defaultGroupName"), inviteOnly(false), topic("defaultTopic"),
          owner(nullptr), password_on(false), password("defaultPassword"), member_limit_on(false) {}

    ~Group()
    {
        // Destructor doesn't delete membersList elements
        // Ownership is handled by the container or elsewhere
    }
};

struct clientDetails
{
    int32_t clientfd; // client file descriptor
    int32_t serverfd; // server file descriptor
    std::vector<singleClient *> clientList;
    clientDetails(void)
    { // initializing the variable
        this->clientfd = -1;
        this->serverfd = -1;
    }
};

const int port = 8081;
const char ip[] = "127.0.0.1"; // for local host
// const ip[]="0.0.0.0"; // for allowing all incomming connection from internet
const int backlog = 5; // maximum number of connection allowed
const char password[] = "bean";

void find_and_send_to_group(std::vector<Group *> groupList, std::string src_string)
{
    char delimiter = ' ';

    std::string GroupName = src_string.substr(0, src_string.find(' '));
    std::string message = src_string.substr(src_string.find(' ') + 1, src_string.length() - 1);
    message += "\0";

    for (int i = 0; i < (int)groupList.size(); i++)
    {
        if (groupList[i]->groupName == GroupName)
        {
            std::cout << "Found Group with name: " << GroupName << std::endl;
            Group *target_group = groupList[i];
            for (int j = 0; j < (int)target_group->membersList.size(); j++)
            {
                send(target_group->membersList[j]->clientfd, message.c_str(), strlen(message.c_str()), MSG_DONTROUTE);
            }
            return;
        }
    }
    std::cout << "No Group with name " << GroupName << " found." << std::endl;
}

void join_group(std::vector<Group *> &groupList, std::string targetGroup, singleClient *client)
{
    for (int i = 0; i < (int)groupList.size(); i++)
    {
        if (groupList[i]->groupName == targetGroup)
        {
            std::cout << "Found Group with name: " << targetGroup << std::endl;
            groupList[i]->membersList.push_back(client);
            client->chatgroupList.push_back(groupList[i]);
            return;
        }
    }
    std::cout << "No Group with name " << targetGroup << " found." << std::endl;
}

void send_to_user(clientDetails *client, std::string src_string)
{
    std::cout << "Command: Send To Use\n";
    char delimiter = ' ';

    std::string target_username = src_string.substr(0, src_string.find(' '));
    std::string message = src_string.substr(src_string.find(' ') + 1, (int)src_string.length() - 1);
    message += "\0";

    std::cout << "username: " << target_username << ", sends " << message << std::endl;

    for (int i = 0; i < (int)client->clientList.size(); i++)
    {
        if (client->clientList[i]->username.compare(target_username) == 0)
        {
            send(client->clientList[i]->clientfd, message.c_str(), strlen(message.c_str()), MSG_DONTROUTE);
        }
    }
}

void create_group(clientDetails *client, std::vector<Group *> &groupList, singleClient *current_client, std::string newGroupName)
{
    Group *newGroup = new Group();
    newGroup->groupName = newGroupName;
    newGroup->membersList.push_back(current_client);
    newGroup->owner = current_client;
    current_client->chatgroupList.push_back(newGroup);
    groupList.push_back(newGroup); // Ensure this modifies the original vector
}

void print_status(clientDetails *clientInfo)
{
    std::cout << "/*-------------------*/\n";
    std::cout << "/*-     status      -*/\n";
    std::cout << "/*-------------------*/\n";
    std::cout << "Clientfd: " << clientInfo->clientfd << std::endl;
    std::cout << "Serverfd: " << clientInfo->serverfd << std::endl;
    std::cout << "/*-------------------*/\n";
    std::cout << "Client List\n";
    for (auto tempClient : clientInfo->clientList)
    {
        if (tempClient->log_in_status == true)
        {
            std::cout << "Clientfd: " << tempClient->clientfd << " | username: " << tempClient->username << " | logged in\n";

            std::cout << "Groups joined:\n";
            for (auto group : tempClient->chatgroupList)
            {
                if (group->inviteOnly == true)
                {
                    std::cout << " -    " << group->groupName << " | " << group->topic << "| private group";
                }
                else
                {
                    std::cout << " -    " << group->groupName << " | " << group->topic << "| public group";
                }
            }
        }
        else
        {
            std::cout << "Clientfd: " << tempClient->clientfd << " | username: " << tempClient->username << " | logged out\n";
            std::cout << "Groups joined:\n";
            for (auto group : tempClient->chatgroupList)
            {
                if (group->inviteOnly == true)
                {
                    std::cout << " -    " << group->groupName << " | " << group->topic << "| private group";
                }
                else
                {
                    std::cout << " -    " << group->groupName << " | " << group->topic << "| public group";
                }
            }
        }
    }
}

void add_to_group(clientDetails *clientInfo, std::vector<Group *> &groupList, std::string src_string, singleClient current_client)
{
    std::string GroupName = src_string.substr(0, src_string.find(' '));
    std::string newUser = src_string.substr(src_string.find(' ') + 1, src_string.length() - 1);

    for (int i = 0; i < (int)clientInfo->clientList.size(); i++)
    {
        if (clientInfo->clientList[i]->username == newUser)
        {
            std::cout << "client exist\n";
            for (int j = 0; j < (int)groupList.size(); j++)
            {
                if (groupList[j]->groupName == GroupName)
                {
                    std::cout << "group exists\n";
                    clientInfo->clientList[i]->chatgroupList.push_back(groupList[j]);
                    groupList[j]->membersList.push_back(clientInfo->clientList[i]);
                    std::string result = "Added " + newUser + " to " + GroupName;
                    send(clientInfo->clientfd, result.c_str(), strlen(result.c_str()), MSG_DONTROUTE);

                    return;
                }
            }
            std::cout << "Group: " << GroupName << " does not exist\n";
        }
    }
    std::cout << "new Member: " << newUser << " does not exist\n";

    std::string buffer = "Failed to add" + newUser + " to " + GroupName + ". Try again\n";
    std::cout << "buffer: " << buffer << std::endl;
    send(clientInfo->clientfd, buffer.c_str(), strlen(buffer.c_str()), MSG_DONTROUTE);
}

int main()
{
    clientDetails *clientInfo = new clientDetails();
    std::vector<Group *> groupList;

    clientInfo->serverfd = socket(AF_INET, SOCK_STREAM, 0); // for tcp connection
    // error handling
    if (clientInfo->serverfd <= 0)
    {
        std::cerr << "socket creation error\n";
        delete clientInfo;
        exit(1);
    }
    else
    {
        std::cout << "socket created\n";
    }
    // setting serverFd to allow multiple connection
    int opt = 1;
    if (setsockopt(clientInfo->serverfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof opt) < 0)
    {
        std::cerr << "setSocketopt error\n";
        delete clientInfo;
        exit(2);
    }

    // setting the server address
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serverAddr.sin_addr);
    // binding the server address
    if (bind(clientInfo->serverfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        std::cerr << "bind error\n";
        delete clientInfo;
        exit(3);
    }
    else
    {
        std::cout << "server binded\n";
    }
    // listening to the port
    if (listen(clientInfo->serverfd, backlog) < 0)
    {
        std::cerr << "listen error\n";
        delete clientInfo;
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
        FD_SET(clientInfo->serverfd, &readfds);
        maxfd = clientInfo->serverfd;
        // copying the client list to readfds
        // so that we can listen to all the client
        for (auto currentClient : clientInfo->clientList)
        {

            int32_t sd = currentClient->clientfd;
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
         * if something happen on clientInfo->serverfd then it means its
         * new connection request
         */
        if (FD_ISSET(clientInfo->serverfd, &readfds))
        {
            clientInfo->clientfd = accept(clientInfo->serverfd, (struct sockaddr *)NULL, NULL);
            if (clientInfo->clientfd < 0)
            {
                std::cerr << "accept error\n";
                continue;
            }
            // adding client to list

            singleClient *temp_client = new singleClient();
            temp_client->clientfd = clientInfo->clientfd;
            temp_client->log_in_status = false;
            clientInfo->clientList.push_back(temp_client);
            std::cout << "new client connected\n";
            std::cout << "new connection, socket fd is " << clientInfo->clientfd << ", ip is: "
                      << inet_ntoa(serverAddr.sin_addr) << ", port: " << ntohs(serverAddr.sin_port) << "\n";

            std::string buffer = "Login:\n";
            send(clientInfo->clientfd, buffer.c_str(), strlen(buffer.c_str()), MSG_DONTROUTE);
            /*
             * std::thread t1(handleConnection, clientInfo);
             * t1.detach();
             *handle the new connection in new thread
             */
        }
        /*
         * else some io operation on some socket
         */

        // for storing the recive message
        char message[1024];
        for (int i = 0; i < clientInfo->clientList.size(); ++i)
        {
            memset(message, 0, 1024);
            sd = clientInfo->clientList[i]->clientfd;
            singleClient *current_client = clientInfo->clientList[i];
            if (FD_ISSET(sd, &readfds))
            {
                valread = read(sd, message, 1024);
                if (valread > 0)
                {
                    message[valread] = '\0'; // Null-terminate the message
                }

                // check if client disconnected
                if (valread == 0)
                {
                    std::cout << "client disconnected\n";

                    getpeername(sd, (struct sockaddr *)&serverAddr, (socklen_t *)&serverAddr);
                    // getpeername name return the address of the client (sd)

                    std::cout << "host disconnected, ip: " << inet_ntoa(serverAddr.sin_addr) << ", port: " << ntohs(serverAddr.sin_port) << "\n";
                    close(sd);
                    /* remove the client from the list */
                    clientInfo->clientList.erase(clientInfo->clientList.begin() + i);
                }

                int message_len = strlen(message);

                char temp_message[1024];

                if (message_len > 2)
                {
                    strcpy(temp_message, message);
                    temp_message[message_len - 1] = '\0';
                    std::cout << "temp_message:  " << temp_message << std::endl;
                    if ((strcmp(temp_message, "exit") == 0) || (strcmp(temp_message, "quit") == 0))
                    {
                        std::cout << "client disconnected\n";

                        getpeername(sd, (struct sockaddr *)&serverAddr, (socklen_t *)&serverAddr);
                        // getpeername name return the address of the client (sd)

                        std::cout << "host disconnected, ip: " << inet_ntoa(serverAddr.sin_addr) << ", port: " << ntohs(serverAddr.sin_port) << "\n";
                        close(sd);
                        /* remove the client from the list */
                        clientInfo->clientList.erase(clientInfo->clientList.begin() + i);
                    }
                    else
                    {
                        if (current_client->log_in_status == false)
                        {
                            if (strcmp(temp_message, password) == 0)
                            {
                                std::cout << "successful login\n";
                                current_client->log_in_status = true;
                                std::string buffer = "Successful login\n";
                                send(clientInfo->clientfd, buffer.c_str(), strlen(buffer.c_str()), MSG_DONTROUTE);
                            }
                            else
                            {
                                std::cout << "failed login\n";
                                std::string temp(temp_message);
                                std::string buffer = temp + " is incorrect. Try again\n";
                                std::cout << "buffer: " << buffer << std::endl;
                                send(clientInfo->clientfd, buffer.c_str(), strlen(buffer.c_str()), MSG_DONTROUTE);
                            }
                        }
                        else
                        {
                            std::cout << "message from client: " << temp_message << "\n";
                            std::string temp(temp_message);
                            if (temp.compare(0, 12, "Set Username") == 0)
                            {
                                current_client->username = temp.substr(13, temp.length());
                                std::cout << "Client username set to: " << current_client->username << std::endl;
                            }
                            else if (temp.compare(0, 6, "STATUS") == 0)
                            {
                                print_status(clientInfo);
                            }
                            else if (temp.compare(0, 12, "Send To User") == 0)
                            {
                                send_to_user(clientInfo, temp.substr(13, (int)temp.length() - 1));
                            }
                            else if (temp.compare(0, 12, "Create Group") == 0)
                            {
                                create_group(clientInfo, groupList, current_client, temp.substr(13, temp.length()));
                            }
                            else if (temp.compare(0, 13, "Send To Group") == 0)
                            {
                                find_and_send_to_group(groupList, temp.substr(14, (int)temp.length() - 1));
                            }
                            else if (temp.compare(0, 10, "Join Group") == 0)
                            {
                                join_group(groupList, temp.substr(11, (int)temp.length() - 1), current_client);
                            }
                            else if (temp.compare(0, 12, "Add To Group") == 0)
                            {
                                add_to_group(clientInfo, groupList, temp.substr(13, (int)temp.length() - 1), *current_client);
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
                else
                {
                    std::cout << "message: " << temp_message << " invalide\n";
                }
            }
        }
    }
    delete clientInfo;
    return 0;
}
