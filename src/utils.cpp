#include "../inc/Server.hpp"

bool isNumber(const std::string& str)
{
  for (std::string::const_iterator i = str.begin(); i != str.end(); ++i)
  {
    if (!std::isdigit(*i))
      return false;
  }
  return true;
}

bool validateInput(int ac, char **av)
{
  Log log;

  if (ac != 3)
  {
    log.nl("Usage: ./ircserv <port> <password>", R);
    return false;
  }
  if (!isNumber(av[1]))
  {
    log.nl("Port must be a number and be between 0 and 65535", R);
    return false;
  }
  int port = atoi(av[1]);
  if (port > 65535 || port < 0)
  {
    log.nl("Port must be a number and be between 0 and 65535", R);
    return false;
  }
  return true;
}

Client* Server::findClient(const std::string &name)
{
  for (size_t i = 0; i < clientList.size(); ++i)
  {
    if (clientList[i]->getUsername() == name || clientList[i]->getNickname() == name)
    {
      return clientList[i];
    }
  }
  return NULL;
}

bool isValidChannelName(const std::string& channelName)
{
  if (channelName.empty())
    return false;

  // Check if the first character is one of '&', '#', '+', '!'
  char firstChar = channelName[0];
  if (firstChar != '&' && firstChar != '#' && firstChar != '+' && firstChar != '!')
    return false;

  // Check the length of the channel name
  if (channelName.length() > 50)
    return false;

  // Check for invalid characters
  for (std::string::const_iterator it = channelName.begin() + 1; it != channelName.end(); ++it)
  {
    char c = *it;
    if (c == ' ' || c == '\x07' || c == ',')
      return false;
  }
  return true;
}

std::vector<std::string> Server::split(const std::string &str, char delimiter)
{
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(str);
  while (std::getline(tokenStream, token, delimiter))
  {
    tokens.push_back(token);
  }
  return tokens;
}

Channel* Server::findChannelByName(const std::string& channelName)
{
  for (std::vector<Channel*>::iterator it = channelList.begin(); it != channelList.end(); ++it)
  {
    if ((*it)->getName() == channelName)
    {
      return *it;
    }
  }
  return NULL;
}

Channel* Server::createChannel(const std::string& channelName, const std::string& key)
{
  Channel* newChannel;
  if (key.empty())
  {
    newChannel = new Channel(channelName);
  }
  else
  {
    newChannel = new Channel(channelName, key);
  }
  channelList.push_back(newChannel);
  return newChannel;
}

void Server::sendJoinMessage(Client* client, Channel* channel)
{
  std::string msg = Y;
  msg.append(client->getNickname() + "!" + client->getUsername() + "@" + _servName);
  msg.append(RESET);
  msg.append(" joined ");
  msg.append(B);
  msg.append(channel->getName() + "\n");
  msg.append(RESET);

  const std::set<Client*>& users = channel->getUsers();
  for (std::set<Client*>::const_iterator it = users.begin(); it != users.end(); ++it)
  {
    send((*it)->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
  }
}

void Server::sendPartMessage(Client *client, Channel *channel)
{
  std::string msg = ":" + client->getNickname() + "!" + client->getUsername() + "@" + _servName + " PART " + channel->getName() + "\n";
  const std::set<Client*>& users = channel->getUsers();
  for (std::set<Client*>::const_iterator it = users.begin(); it != users.end(); ++it)
  {
    send((*it)->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
  }
}

// void Server::removeClient(Client *client)
// {
//   for (std::vector<Client*>::iterator it = clientList.begin(); it != clientList.end(); ++it)
//   {
//     if (*it == client)
//     {
//       clientList.erase(it);
//       delete client;
//       break;
//     }
//   }
// }
