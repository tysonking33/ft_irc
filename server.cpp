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
    std::vector<singleClient *> membersList;  // Use raw pointers to singleClient
    std::vector<singleClient *> OperatorList; // Use raw pointers to singleClient
    std::vector<singleClient *> currentlySignedIn;
    bool inviteOnly;
    std::string topic;
    bool topic_bool;
    singleClient *owner; // Raw pointer to owner
    bool password_on;
    std::string password;
    bool member_limit_on;
    int memberLimit;
    int memberCount;

    Group()
        : groupName("defaultGroupName"), inviteOnly(false), topic("defaultTopic"), topic_bool(false),
          owner(nullptr), password_on(false), password("defaultPassword"), member_limit_on(false), memberCount(0) {}

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

void print_status(const clientDetails &details)
{
    std::cout << "Server Status:\n";
    std::cout << "----------------------------------------\n";
    std::cout << "Server File Descriptor: " << details.serverfd << "\n";
    std::cout << "Connected Clients:\n";

    for (size_t i = 0; i < details.clientList.size(); ++i)
    {
        singleClient *client = details.clientList[i];
        if (client)
        {
            std::cout << "Client ID: " << client->clientId << "\n";
            std::cout << "Client FD: " << client->clientfd << "\n";
            std::cout << "Username: " << client->username << "\n";
            std::cout << "Logged In: " << (client->log_in_status ? "Yes" : "No") << "\n";
            std::cout << "Chat Groups:\n";

            for (size_t j = 0; j < client->chatgroupList.size(); ++j)
            {
                Group *group = client->chatgroupList[j];
                if (group)
                {
                    std::cout << "- Group Name: " << group->groupName << "\n";
                    std::cout << "  Topic: " << group->topic << "\n";
                    std::cout << "  Invite Only: " << (group->inviteOnly ? "Yes" : "No") << "\n";
                    std::cout << "  Password Protected: " << (group->password_on ? "Yes" : "No") << "\n";
                    std::cout << "  Member Limit: "
                              << (group->member_limit_on ? group->memberLimit : -1)
                              << "\n";
                }
            }
        }
        std::cout << "----------------------------------------\n";
    }
}

void find_and_send_to_group(std::vector<Group *> groupList, std::string src_string)
{
    char delimiter = ' ';
    size_t pos = src_string.find(delimiter);
    
    std::string GroupName = src_string.substr(0, pos);
    std::string message = src_string.substr(pos + 1);

    for (size_t i = 0; i < groupList.size(); i++)
    {
        if (groupList[i]->groupName == GroupName)
        {
            std::cout << "Found Group with name: " << GroupName << std::endl;
            Group *target_group = groupList[i];
            for (size_t j = 0; j < target_group->membersList.size(); j++)
            {
                send(target_group->membersList[j]->clientfd, message.c_str(), message.length(), MSG_DONTROUTE);
            }
            return;
        }
    }
    std::cout << "No Group with name " << GroupName << " found." << std::endl;
}

void join_group(std::vector<Group *> &groupList, const std::string &targetGroup, singleClient *client)
{
    for (size_t i = 0; i < groupList.size(); ++i)
    {
        if (groupList[i]->groupName == targetGroup)
        {
            std::cout << "Found Group with name: " << targetGroup << std::endl;

            if (groupList[i]->member_limit_on && groupList[i]->memberCount >= groupList[i]->memberLimit)
            {
                std::cout << "Cannot join group: Member limit reached." << std::endl;
                return;
            }

            groupList[i]->membersList.push_back(client);
            groupList[i]->memberCount++;
            client->chatgroupList.push_back(groupList[i]);

            std::cout << "Client added to group: " << targetGroup << std::endl;
            return;
        }
    }
    std::cout << "No Group with name " << targetGroup << " found." << std::endl;
}

void send_to_user(clientDetails *client, std::string src_string)
{
    std::cout << "Command: Send To User\n";
    char delimiter = ' ';
    size_t spacePos = src_string.find(delimiter);
    
    std::string target_username = src_string.substr(0, spacePos);
    std::string message = src_string.substr(spacePos + 1);

    for (size_t i = 0; i < client->clientList.size(); i++)
    {
        if (client->clientList[i]->username == target_username)
        {
            send(client->clientList[i]->clientfd, message.c_str(), message.length(), MSG_DONTROUTE);
            return;
        }
    }
    std::cout << "No user with username: " << target_username << " found." << std::endl;
}

void create_group(clientDetails *client, std::vector<Group *> &groupList, singleClient *current_client, std::string newGroupName)
{
    Group *newGroup = new Group();
    newGroup->groupName = newGroupName;
    newGroup->membersList.push_back(current_client);
    newGroup->owner = current_client;
    newGroup->OperatorList.push_back(current_client);
    newGroup->currentlySignedIn.push_back(current_client);
    newGroup->memberCount = 1;  // As the owner is added
    current_client->chatgroupList.push_back(newGroup);
    groupList.push_back(newGroup);
}

void add_to_group(clientDetails *clientInfo, std::vector<Group *> &groupList, const std::string &src_string, singleClient *current_client)
{
    size_t spacePos = src_string.find(' ');
    if (spacePos == std::string::npos)
    {
        std::cerr << "Invalid input format for Add to Group\n";
        return;
    }

    std::string GroupName = src_string.substr(0, spacePos);
    std::string newUser = src_string.substr(spacePos + 1);

    for (size_t i = 0; i < clientInfo->clientList.size(); ++i)
    {
        if (clientInfo->clientList[i]->username == newUser)
        {
            std::cout << "Client exists: " << newUser << std::endl;

            for (size_t j = 0; j < groupList.size(); ++j)
            {
                Group *group = groupList[j];
                if (group->groupName == GroupName)
                {
                    bool isOperator = false;
                    for (size_t k = 0; k < group->OperatorList.size(); ++k)
                    {
                        if (group->OperatorList[k] == current_client)
                        {
                            isOperator = true;
                            break;
                        }
                    }

                    if (!isOperator)
                    {
                        std::cout << "Current client is not an operator in the group." << std::endl;
                        std::string buffer = "You must be an operator to add members to the group: " + GroupName + ".\n";
                        send(clientInfo->clientfd, buffer.c_str(), buffer.length(), MSG_DONTROUTE);
                        return;
                    }

                    if (group->member_limit_on && group->memberCount >= group->memberLimit)
                    {
                        std::cout << "Group member limit reached." << std::endl;
                        std::string buffer = "Cannot add " + newUser + " to " + GroupName + ": Member limit reached.\n";
                        send(clientInfo->clientfd, buffer.c_str(), buffer.length(), MSG_DONTROUTE);
                        return;
                    }

                    clientInfo->clientList[i]->chatgroupList.push_back(group);
                    group->membersList.push_back(clientInfo->clientList[i]);
                    group->memberCount++;

                    std::string result = "Added " + newUser + " to " + GroupName + ".\n";
                    send(clientInfo->clientfd, result.c_str(), result.length(), MSG_DONTROUTE);
                    return;
                }
            }
            std::cout << "Group: " << GroupName << " does not exist." << std::endl;
        }
    }
    std::cout << "New member: " << newUser << " does not exist." << std::endl;

    std::string buffer = "Failed to add " + newUser + " to " + GroupName + ". Try again.\n";
    send(clientInfo->clientfd, buffer.c_str(), buffer.length(), MSG_DONTROUTE);
}

void kick_from_group(clientDetails *clientInfo, std::vector<Group *> &groupList, const std::string &src_string, singleClient *current_client)
{
    size_t spacePos = src_string.find(' ');
    if (spacePos == std::string::npos)
    {
        std::cerr << "Invalid input format for Kick From Group\n";
        return;
    }

    std::string GroupName = src_string.substr(0, spacePos);
    std::string targetUser = src_string.substr(spacePos + 1);

    for (size_t j = 0; j < groupList.size(); ++j)
    {
        Group *group = groupList[j];

        if (group->groupName == GroupName)
        {
            std::cout << "Group exists: " << GroupName << std::endl;

            bool isOperator = false;
            for (size_t k = 0; k < group->OperatorList.size(); ++k)
            {
                if (group->OperatorList[k] == current_client)
                {
                    isOperator = true;
                    break;
                }
            }

            if (!isOperator)
            {
                std::cout << "Current client is not an operator in the group." << std::endl;
                std::string buffer = "You must be an operator to kick members from the group: " + GroupName + ".\n";
                send(clientInfo->clientfd, buffer.c_str(), buffer.length(), MSG_DONTROUTE);
                return;
            }

            for (size_t i = 0; i < group->membersList.size(); ++i)
            {
                if (group->membersList[i]->username == targetUser)
                {
                    std::cout << "Found user: " << targetUser << std::endl;

                    group->membersList.erase(group->membersList.begin() + i);
                    group->memberCount--;

                    std::string result = targetUser + " removed from " + GroupName + "\n";
                    send(clientInfo->clientfd, result.c_str(), result.length(), MSG_DONTROUTE);
                    return;
                }
            }

            std::cout << "User " << targetUser << " not found in group " << GroupName << std::endl;
            break;
        }
    }

    std::string buffer = "Failed to remove " + targetUser + " from " + GroupName + ". Try again.\n";
    std::cout << "Buffer: " << buffer << std::endl;
    send(clientInfo->clientfd, buffer.c_str(), buffer.length(), MSG_DONTROUTE);
}

bool isClientValidForGroup(std::vector<Group *> &groupList, singleClient *current_client, const std::string &groupName)
{
    // Iterate over the groupList to find the corresponding group
    for (int j = 0; j < (int)groupList.size(); j++)
    {
        if (groupList[j]->groupName == groupName)
        {
            // Check if the client is logged in and part of the group
            bool isLoggedIn = current_client->log_in_status;
            bool isPartOfGroup = std::find(groupList[j]->currentlySignedIn.begin(), groupList[j]->currentlySignedIn.end(), current_client) != groupList[j]->currentlySignedIn.end();

            if (!isLoggedIn || !isPartOfGroup)
            {
                std::cerr << "Client is not logged in or not part of the group.\n";
                return false;
            }

            // Check for password protection if enabled
            if (groupList[j]->password_on && groupList[j]->password != "")
            {
                // Prompt for password (this part depends on your system of handling password input)
                std::string enteredPassword;
                std::cout << "Enter password for group " << groupName << ": ";
                std::cin >> enteredPassword;

                if (enteredPassword != groupList[j]->password)
                {
                    std::cerr << "Incorrect password.\n";
                    return false;
                }
            }

            // Client is valid for the group
            return true;
        }
    }

    std::cerr << "Group not found.\n";
    return false;
}

void remove_operator_privilege(clientDetails *clientInfo, std::vector<Group *> &groupList, std::string src_string, singleClient current_client)
{
    size_t spacePos = src_string.find(' ');
    if (spacePos == std::string::npos)
    {
        std::cerr << "Invalid input format for Remove Operator Privilege\n";
        return;
    }
    std::string GroupName = src_string.substr(0, spacePos);
    std::string targetUser = src_string.substr(spacePos + 1);

    for (int j = 0; j < (int)groupList.size(); j++)
    {
        if (groupList[j]->groupName == GroupName)
        {
            bool isOperator = false;
            for (int i = 0; i < (int)groupList[j]->OperatorList.size(); i++)
            {
                if (groupList[j]->OperatorList[i]->clientId == current_client.clientId)
                {
                    isOperator = true;
                    break;
                }
            }

            if (isOperator)
            {
                std::cout << "group exists\n";
                for (int i = 0; i < (int)groupList[j]->OperatorList.size(); i++)
                {
                    if (groupList[j]->OperatorList[i]->username == targetUser)
                    {
                        std::cout << "found user\n";
                        for (int a = i; a < (int)groupList[j]->OperatorList.size() - 1; ++a)
                        {
                            groupList[j]->OperatorList[a] = groupList[j]->OperatorList[a + 1];
                        }
                        groupList[j]->OperatorList.pop_back();
                        std::string result = targetUser + " removed from " + GroupName + "'s privilege\n";
                        std::cout << result;
                        return;
                    }
                }
                std::cout << targetUser << " not found in operator list.\n";
            }
            else
            {
                std::cout << "You do not have permission to remove operator privileges.\n";
            }
        }
    }
}

void set_remove_invite_only(clientDetails *clientInfo, std::vector<Group *> &groupList, std::string src_string, singleClient current_client)
{
    size_t spacePos = src_string.find(' ');
    if (spacePos == std::string::npos)
    {
        std::cerr << "Invalid input format for Set/Remove Invite Only\n";
        return;
    }
    std::string GroupName = src_string.substr(0, spacePos);
    std::string set = src_string.substr(spacePos + 1);

    for (int j = 0; j < (int)groupList.size(); j++)
    {
        if (groupList[j]->groupName == GroupName)
        {
            bool isOperator = false;
            for (int i = 0; i < (int)groupList[j]->OperatorList.size(); i++)
            {
                if (groupList[j]->OperatorList[i]->clientId == current_client.clientId)
                {
                    isOperator = true;
                    break;
                }
            }

            if (isOperator)
            {
                std::cout << "group exists\n";
                if (set == "set")
                    groupList[j]->inviteOnly = true;
                else if (set == "remove")
                    groupList[j]->inviteOnly = false;
            }
            else
            {
                std::cout << "You do not have permission to change invite-only setting.\n";
            }
        }
    }
}

void set_channel_password(clientDetails *clientInfo, std::vector<Group *> &groupList, std::string src_string, singleClient current_client)
{
    std::istringstream stream(src_string);

    // Variables to hold the parsed words
    std::string GroupName, set, newPassword;

    // Extract words from the stream
    stream >> GroupName >> set >> newPassword;

    for (int j = 0; j < (int)groupList.size(); j++)
    {
        if (groupList[j]->groupName == GroupName)
        {
            bool isOperator = false;
            for (int i = 0; i < (int)groupList[j]->OperatorList.size(); i++)
            {
                if (groupList[j]->OperatorList[i]->clientId == current_client.clientId)
                {
                    isOperator = true;
                    break;
                }
            }

            if (isOperator)
            {
                std::cout << "group exists\n";
                if (set == "set")
                {
                    groupList[j]->password_on = true;
                    groupList[j]->password = newPassword;
                }
                else if (set == "remove")
                {
                    groupList[j]->password_on = false;
                    groupList[j]->password = "";
                }
            }
            else
            {
                std::cout << "You do not have permission to set or remove the channel password.\n";
            }
        }
    }
}

void set_user_limit(clientDetails *clientInfo, std::vector<Group *> &groupList, std::string src_string, singleClient current_client)
{
    std::istringstream stream(src_string);

    // Variables to hold the parsed words
    std::string GroupName, limit_bool, str_number;

    // Extract words from the stream
    stream >> GroupName >> limit_bool >> str_number;

    for (int j = 0; j < (int)groupList.size(); j++)
    {
        if (groupList[j]->groupName == GroupName)
        {
            bool isOperator = false;
            for (int i = 0; i < (int)groupList[j]->OperatorList.size(); i++)
            {
                if (groupList[j]->OperatorList[i]->clientId == current_client.clientId)
                {
                    isOperator = true;
                    break;
                }
            }

            if (isOperator)
            {
                std::cout << "group exists\n";
                if (limit_bool == "set")
                {
                    groupList[j]->member_limit_on = true;
                    groupList[j]->memberCount = stoi(str_number);
                }
                else if (limit_bool == "remove")
                {
                    groupList[j]->member_limit_on = false;
                    groupList[j]->memberCount = 2147483647;
                }
            }
            else
            {
                std::cout << "You do not have permission to change user limit.\n";
            }
        }
    }
}

void set_remove_topic(clientDetails *clientInfo, std::vector<Group *> &groupList, std::string src_string, singleClient current_client)
{
    std::istringstream stream(src_string);

    // Variables to hold the parsed words
    std::string GroupName, topic_bool, str_topic;

    // Extract words from the stream
    stream >> GroupName >> topic_bool >> str_topic;

    for (int j = 0; j < (int)groupList.size(); j++)
    {
        if (groupList[j]->groupName == GroupName)
        {
            bool isOperator = false;
            for (int i = 0; i < (int)groupList[j]->OperatorList.size(); i++)
            {
                if (groupList[j]->OperatorList[i]->clientId == current_client.clientId)
                {
                    isOperator = true;
                    break;
                }
            }

            if (isOperator)
            {
                std::cout << "group exists\n";
                if (topic_bool == "set")
                {
                    groupList[j]->topic_bool = true;
                    groupList[j]->topic = str_topic;
                }
                else if (topic_bool == "remove")
                {
                    groupList[j]->topic_bool = false;
                    groupList[j]->topic = "";
                }
            }
            else
            {
                std::cout << "You do not have permission to change the topic.\n";
            }
        }
    }
}

void login_to_group(clientDetails *clientInfo, std::vector<Group *> &groupList, std::string src_string, singleClient current_client)
{
    std::istringstream stream(src_string);

    // Variables to hold the parsed words
    std::string GroupName, password_guess;

    // Extract words from the stream
    stream >> GroupName >> password_guess;

    for (int j = 0; j < (int)groupList.size(); j++)
    {
        if (groupList[j]->groupName == GroupName)
        {
            if (groupList[j]->password_on == false)
            {
                groupList[j]->currentlySignedIn.push_back(&current_client);
            }
            else
            {
                if (groupList[j]->password == password_guess)
                    groupList[j]->currentlySignedIn.push_back(&current_client);
                else
                    std::cout << "failed to login\n";
            }
        }
    }
}

void change_group_topic(clientDetails *clientInfo, std::vector<Group *> &groupList, std::string src_string, singleClient current_client)
{
    size_t spacePos = src_string.find(' ');
    if (spacePos == std::string::npos)
    {
        std::cerr << "Invalid input format for Change Group Topic\n";
        return;
    }
    std::string GroupName = src_string.substr(0, spacePos);
    std::string newTopic = src_string.substr(spacePos + 1);

    // Loop through the group list to find the group
    for (int j = 0; j < (int)groupList.size(); j++)
    {
        if (groupList[j]->groupName == GroupName)
        {
            // Check if the current client is in the operator list
            bool isOperator = false;
            for (int i = 0; i < (int)groupList[j]->OperatorList.size(); i++)
            {
                if (groupList[j]->OperatorList[i]->clientId == current_client.clientId)
                {
                    isOperator = true;
                    break;
                }
            }

            if (isOperator)
            {
                std::cout << "Group exists, and " << current_client.username << " is an operator\n";
                groupList[j]->topic = newTopic; // Change the group topic
                std::cout << "Topic changed to: " << newTopic << std::endl;
            }
            else
            {
                std::cout << "You do not have permission to change the topic. Only operators can change the topic.\n";
            }
            return;
        }
    }
    std::cout << "Group: " << GroupName << " does not exist\n";
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
        for (int i = 0; i < (int)clientInfo->clientList.size(); i++)
        {

            int32_t sd = clientInfo->clientList[i]->clientfd;
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

                if (message_len > 1)
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
                                print_status(*clientInfo);
                            }
                            else if (temp.compare(0, 12, "Send To User") == 0)
                            {
                                send_to_user(clientInfo, temp.substr(13, (int)temp.length() - 1));
                            }
                            else if (temp.compare(0, 12, "Create Group") == 0)
                            {
                                create_group(clientInfo, groupList, current_client, temp.substr(13, temp.length()));
                            }
                            else if (temp.compare(0, 14, "Login To Group") == 0)
                            {
                                login_to_group(clientInfo, groupList, temp.substr(15, (int)temp.length() - 1), *current_client);
                            }
                            else if (temp.compare(0, 13, "Send To Group") == 0)
                            {
                                // Ensure the client is logged in, and is part of the group (password check if needed)
                                if (isClientValidForGroup(groupList, current_client, temp.substr(14)))
                                {
                                    find_and_send_to_group(groupList, temp.substr(14, (int)temp.length() - 1));
                                }
                            }
                            else if (temp.compare(0, 10, "Join Group") == 0)
                            {
                                // Ensure the client is logged in, and is part of the group (password check if needed)
                                if (isClientValidForGroup(groupList, current_client, temp.substr(11)))
                                {
                                    join_group(groupList, temp.substr(11, (int)temp.length() - 1), current_client);
                                }
                            }
                            else if (temp.compare(0, 6, "INVITE") == 0)
                            {
                                // Ensure the client is logged in, and is part of the group (password check if needed)
                                if (isClientValidForGroup(groupList, current_client, temp.substr(7)))
                                {
                                    add_to_group(clientInfo, groupList, temp.substr(7, (int)temp.length() - 1), current_client);
                                }
                            }
                            else if (temp.compare(0, 4, "KICK") == 0)
                            {
                                // Ensure the client is logged in, and is part of the group (password check if needed)
                                if (isClientValidForGroup(groupList, current_client, temp.substr(5)))
                                {
                                    kick_from_group(clientInfo, groupList, temp.substr(5, (int)temp.length() - 1), current_client);
                                }
                            }
                            else if (temp.compare(0, 5, "TOPIC") == 0)
                            {
                                // Ensure the client is logged in, and is part of the group (password check if needed)
                                if (isClientValidForGroup(groupList, current_client, temp.substr(6)))
                                {
                                    change_group_topic(clientInfo, groupList, temp.substr(6, (int)temp.length() - 1), *current_client);
                                }
                            }
                            else if (temp.compare(0, 4, "MODE") == 0)
                            {
                                std::string new_temp = temp.substr(5, (int)temp.length() - 1);
                                // Ensure the client is logged in, and is part of the group (password check if needed)
                                if (isClientValidForGroup(groupList, current_client, new_temp))
                                {
                                    if (new_temp[0] == 'i')
                                    {
                                        new_temp = new_temp.substr(2, (int)new_temp.length() - 1);
                                        // set/remove invite only channel
                                        set_remove_invite_only(clientInfo, groupList, new_temp, *current_client);
                                    }
                                    else if (new_temp[0] == 't')
                                    {
                                        new_temp = new_temp.substr(2, (int)new_temp.length() - 1);
                                        // set/remove restrictions of the TOPIC command to channel operator
                                        set_remove_topic(clientInfo, groupList, new_temp, *current_client);
                                    }
                                    else if (new_temp[0] == 'k')
                                    {
                                        new_temp = new_temp.substr(2, (int)new_temp.length() - 1);
                                        // set/remove channel key/password
                                        set_channel_password(clientInfo, groupList, new_temp, *current_client);
                                    }
                                    else if (new_temp[0] == 'o')
                                    {
                                        new_temp = new_temp.substr(2, (int)new_temp.length() - 1);
                                        // give/take channel operator privilege
                                        remove_operator_privilege(clientInfo, groupList, new_temp, *current_client);
                                    }
                                    else if (new_temp[0] == 'l')
                                    {
                                        new_temp = new_temp.substr(2, (int)new_temp.length() - 1);
                                        // set/remove the user limit to channel
                                        set_user_limit(clientInfo, groupList, new_temp, *current_client);
                                    }
                                }
                            }
                            else
                            {
                                std::cout << "message: " << temp_message << std::endl;
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
    delete clientInfo;
    return 0;
}
