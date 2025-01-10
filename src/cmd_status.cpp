#include "../inc/Server.hpp"

void Server::printStatus(Client *client)
{
  if (client == NULL)
  {
    std::string msg = "Client is null\n";
    send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
    return;
  }

  std::string msg;
  msg.append("--- Client Status ---\n");
  msg.append("Username  : " + client->getUsername() + "\n");
  msg.append("First Name: " + client->getFirstName() + "\n");
  msg.append("Last Name : " + client->getLastName() + "\n");
  msg.append("Nickname  : " + client->getNickname() + "\n");
  msg.append("Logged In : " + std::string(client->isLoggedIn() ? "Yes" : "No") + "\n");
  msg.append("Registered: " + std::string(client->isRegistered() ? "Yes" : "No") + "\n");
  msg.append("Operator  : " + std::string(client->getOperator() ? "Yes" : "No") + "\n");

  msg.append("--- Channels Joined ---\n");
  for (std::vector<Channel*>::const_iterator it = channelList.begin(); it != channelList.end(); ++it)
  {
    if ((*it)->isUserInChannel(client))
    {
      msg.append(" - " + (*it)->getName() + "\n");
    }
  }

  send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
}
