/*
Command: NICK
   Parameters: <nickname>
*/

#include "../inc/Server.hpp"

void Server::setNick(Client *client, const std::vector<std::string> &tokens)
{
  if (tokens.size() != 2)
  {
    std::string msg = "Usage: NICK <nickname>\n";
    send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
    return;
  }

  std::string nickname = tokens[1];

  // Check if username already exists
  for (std::vector<Client *>::iterator it = clientList.begin(); it != clientList.end(); ++it)
  {
    if ((*it)->getNickname() == nickname || (*it)->getUsername() == nickname)
    {
      std::string msg = "\033[0;31mThis name already exists. Please choose another nickname.\033[0;0m\n";
      send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
      return;
    }
  }

  client->setNickname(nickname);
  client->setNickReceived(true);
  std::string msg = "User nickname updated successfully.\n";
  send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);

  // Check if both NICK and USER have been received
  if (client->getUserReceived())
  {
    handleUserLogin(client);
  }
}
