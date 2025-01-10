/*
Command: OPER
   Parameters: <name> <password>

   A normal user uses the OPER command to obtain operator privileges.
   The combination of <name> and <password> are REQUIRED to gain
   Operator privileges.  Upon success, the user will receive a MODE
   message indicating the new user modes.

   Example:

   OPER foo bar                    ; Attempt to register as an operator
                                   using a username of "foo" and "bar"
                                   as the password.
*/

#include "../inc/Server.hpp"

void Server::setOper(Client *client, const std::vector<std::string> &tokens)
{
  if (tokens.size() < 3)
  {
    std::string msg = "\033[0;31mError: Not enough parameters for OPER command.\033[0;0m\n";
    send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
    return;
  }

  const std::string &name = tokens[1];
  std::string password;
  for (size_t i = 2; i < tokens.size(); ++i)
  {
    password += tokens[i];
    if (i != tokens.size() - 1)
    {
      password += " ";
    }
  }
  password.erase(password.find_last_not_of("\n\r\t") + 1);

  if (name == client->getUsername() && password == OPER_PASS)
  {
    client->setOperator(true);
    std::string msg = Y;
    msg.append(client->getUsername());
    msg.append(RESET);
    msg.append("\n: You are now an IRC operator.\n");
    send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);

    // Server log
    log.out(client->getUsername(), B);
    log.nl(" is now an IRC operator.");
  }
  else
  {
    std::string msg = "\033[0;31mError: Incorrect password or Username.\033[0;0m\n";
    send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
  }
}
