#include "../inc/Server.hpp"

std::string Server::sendWelcomeMessage(Client* client)
{
  // <nick>!<user>@<host>
  std::string msg_login = G;
  msg_login.append("\nWelcome to the Internet Relay Network\n");
  msg_login.append(Y);
  msg_login.append(client->getNickname() + "!" + client->getUsername() + "@" + _servName + "\n\n");
  msg_login.append(B);
  msg_login.append("------ Your details ------\n");
  msg_login.append("Username      : ");
  msg_login.append(Y);
  msg_login.append(client->getUsername() + "\n");
  msg_login.append(B);
  msg_login.append("Nickname      : ");
  msg_login.append(Y);
  msg_login.append(client->getNickname() + "\n");
  msg_login.append(B);
  msg_login.append("First Name    : ");
  msg_login.append(Y);
  msg_login.append(client->getFirstName() + "\n");
  msg_login.append(B);
  msg_login.append("Last Name     : ");
  msg_login.append(Y);
  msg_login.append(client->getLastName() + "\n");
  msg_login.append(B);
  msg_login.append("--------------------------\n\n");
  msg_login.append(RESET);
  return msg_login;
}

void Server::handleUserLogin(Client* client)
{
  client->setLoggedIn(true);
  std::string welcomeMsg = sendWelcomeMessage(client);
  send(client->getClientfd(), welcomeMsg.c_str(), welcomeMsg.length(), MSG_DONTROUTE);

  // Server log
  log.out("User ");
  log.out(client->getUsername(), B);
  log.out(" with nickname ");
  log.out(client->getNickname(), B);
  log.nl(" has logged in.");
}
