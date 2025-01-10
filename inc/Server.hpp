#ifndef SERVER_HPP
# define SERVER_HPP

# include <iostream>
# include <cstring>
# include <sstream>
# include <arpa/inet.h>
# include <netinet/in.h>
# include <sys/select.h>
# include <sys/socket.h>
# include <unistd.h>
# include <vector>
# include <string>
# include <sstream>
# include <cctype>
# include <set>

# include "Client.hpp"
# include "Channel.hpp"
# include "Log.hpp"

# define MAX_CLIENTS 5
# define IP_ADDR "127.0.0.1"
# define OPER_PASS "bean"

class Server
{
  public:
    Server(const std::string &name, int port, const std::string &password);
    ~Server();

    void start();
    std::vector<std::string> split(const std::string &str, char delimiter);

  private:
    std::string _servName;
    int _port;
    std::string _password;
    int _serverfd;
    std::vector<Client *> clientList;
    std::vector<Channel *> channelList;
    Log log;
    bool _shutdown;

    void handleClientMessages(fd_set &readfds);
    void handleMessage(Client *client, const std::string &message);
    std::string sendWelcomeMessage(Client* client);
    void handleUserLogin(Client* client);
    Client* findClient(const std::string &name);
    void execute(Client *client, const std::vector<std::string> &tokens, const std::string &message);
    void checkPassword(Client *client, const char *message);
    void setUser(Client *client, const std::vector<std::string> &tokens);
    void setNick(Client *client, const std::vector<std::string> &tokens);
    void setOper(Client *client, const std::vector<std::string> &tokens);
    void sendPrivateMessage(Client* senderClient, const std::vector<std::string> &tokens, const std::string &message);
    void joinChannel(Client* client, const std::vector<std::string> &tokens);
    void sendJoinMessage(Client* client, Channel* channel);
    void leaveAllChannels(Client *client);
    void sendPartMessage(Client *client, Channel *channel);
    Channel* findChannelByName(const std::string& channelName);
    Channel* createChannel(const std::string& channelName, const std::string& key = "");
    void setMode(Client *client, const std::vector<std::string> &tokens);
    void setInvite(Client *client, const std::vector<std::string> &tokens);
    void kick(Client *client, const std::vector<std::string> &tokens);
    void setTopic(Client *client, const std::vector<std::string> &tokens);
    void printStatus(Client *client);
    void sendHelp(Client *client);
    void shutdownServer(Client *client);
    void shutdownServer();
};

// Utility functions
bool isNumber(const std::string &str);
bool validateInput(int ac, char **av);
bool isValidChannelName(const std::string& channelName);

#endif
