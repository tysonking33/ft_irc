/*
Command: PASS
    Parameters: <password>
*/

#include "../inc/Server.hpp"

void Server::checkPassword(Client *client, const char *message)
{
  int sd = client->getClientfd();
  if (strncmp(message, "PASS ", 5) == 0)
  {
    std::string password = message + 5;
    password.erase(password.find_last_not_of("\n\r\t") + 1);
    if (password == _password)
    {
      client->setRegistered(true);
      std::string msg = B;
      msg.append("\nPassword accepted. You are now registered to " + _servName + "!\n\n");
      msg.append(Y);
      msg.append("Please set Username and Nickname\n");
      msg.append("USER <username> <fname> <lname>\n");
      msg.append("NICK <nickname>\n\n");
      msg.append(RESET);
      send(sd, msg.c_str(), msg.length(), MSG_DONTROUTE);

      // Server log
      log.nl("Client has registered.");
      log.out(sd, B);
      log.nl(" has registered.");
    }
    else
    {
      std::string msg = "\033[0;31mIncorrect password. Please try again.\033[0;0m\n";
      send(sd, msg.c_str(), msg.length(), MSG_DONTROUTE);
    }
  }
  else
  {
    std::string msg = "Please enter password to continue.\nPASS <serv_pass>\n";
    send(sd, msg.c_str(), msg.length(), MSG_DONTROUTE);
  }
}
