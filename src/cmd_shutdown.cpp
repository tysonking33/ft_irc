#include "../inc/Server.hpp"

// void Server::shutdownServer(Client *client)
// {
//   if (!client->getOperator())
//   {
//     std::string msg = "\033[0;31mError: You do not have permission to use this command.\033[0;0m\n";
//     send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
//     return;
//   }

//   std::string shutdownMsg = "Server is shutting down. You will be disconnected.\n";

//   // Notify all clients and disconnect them
//   for (std::vector<Client *>::iterator it = clientList.begin(); it != clientList.end(); ++it)
//   {
//     send((*it)->getClientfd(), shutdownMsg.c_str(), shutdownMsg.length(), MSG_DONTROUTE);
//   }

//   _shutdown = true;
// }

// void Server::shutdownServer()
// {
//   // Clear all channels
//   for (std::vector<Channel *>::iterator it = channelList.begin(); it != channelList.end(); ++it)
//   {
//     delete *it;
//   }
//   channelList.clear();

//   // Delete all allocated clients
//   for (std::vector<Client *>::iterator it = clientList.begin(); it != clientList.end(); ++it)
//   {
//     close((*it)->getClientfd());
//     delete *it;
//   }
//   clientList.clear();

//   // Call the destructor directly
//   this->~Server();

//   // Exit the program
//   exit(0);
// }

void Server::shutdownServer(Client *client)
{
  if (!client->getOperator())
  {
    std::string msg = "\033[0;31mError: You do not have permission to use this command.\033[0;0m\n";
    send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
    return;
  }

  std::string shutdownMsg = "Server is shutting down. You will be disconnected.\n";

  // Notify all clients and disconnect them
  for (std::vector<Client *>::iterator it = clientList.begin(); it != clientList.end(); ++it)
  {
    send((*it)->getClientfd(), shutdownMsg.c_str(), shutdownMsg.length(), MSG_DONTROUTE);
    close((*it)->getClientfd()); // Close client connection
  }

  _shutdown = true;
}

void Server::shutdownServer()
{
  // Clear all channels
  for (std::vector<Channel *>::iterator it = channelList.begin(); it != channelList.end(); ++it)
  {
    delete *it;
  }
  channelList.clear();

  // Delete all allocated clients
  for (std::vector<Client *>::iterator it = clientList.begin(); it != clientList.end(); ++it)
  {
    close((*it)->getClientfd()); // Close client connection
    delete *it;
  }
  clientList.clear();

  // Close the server socket
  close(_serverfd);

  // Log the shutdown
  log.nl("Server has been shut down.");
}