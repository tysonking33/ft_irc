#include "../inc/Server.hpp"

void Server::sendHelp(Client *client)
{
  std::string msg = "Available commands:\n\n";
  msg.append("Normal User Commands:\n");
  msg.append("  PASS <password>\n");
  msg.append("  NICK <nickname>\n");
  msg.append("  USER <username> <fname> <lname>\n");
  msg.append("  JOIN <channel> [<key>]\n");
  msg.append("  PRIVMSG <msgtarget> <text to be sent>\n");
  msg.append("  TOPIC <channel>\n");
  msg.append("  INVITE <nickname> <channel>\n");
  msg.append("  STATUS\n");
  msg.append("  HELP\n\n");

  if (client->getOperator())
  {
    msg.append("Operator Commands:\n");
    msg.append("  OPER <username> <password>\n");
    msg.append("  MODE <channel> <( \"-\" / \"+\" )modes> [parameters]\n");
    msg.append("  KICK <channel> <user> [<comment>]\n");
    msg.append("  TOPIC <channel> [<topic>]\n");
    msg.append("  SHUTDOWN\n");
  }

  send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
}
