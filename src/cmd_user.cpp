/*
Command: USER
   Parameters: <user> <mode> <unused> <realname>
*/

#include "../inc/Server.hpp"

void Server::setUser(Client *client, const std::vector<std::string> &tokens)
{
  if (tokens.size() != 4)
  {
    std::string msg = "Usage: USER <username> <fname> <lname>\n";
    send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
    return;
  }

  std::string username = tokens[1];
  std::string fname = tokens[2];
  std::string lname = tokens[3];

  // Check if username already exists
  for (std::vector<Client *>::iterator it = clientList.begin(); it != clientList.end(); ++it)
  {
    if ((*it)->getUsername() == username || (*it)->getNickname() == username)
    {
      std::string msg = "\033[0;31mThis name already exists. Please choose another username.\033[0;0m\n";
      send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
      return;
    }
  }

  client->setUsername(username);
  client->setFirstName(fname);
  client->setLastName(lname);
  client->setUserReceived(true);
  std::string msg = "User information updated successfully.\n";
  send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);

  // Check if both NICK and USER have been received
  if (client->getNickReceived())
  {
    handleUserLogin(client);
  }
}
