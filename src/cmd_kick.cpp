/*
Command: KICK
   Parameters: <channel> *( "," <channel> ) <user> *( "," <user> )
               [<comment>]

   The KICK command can be used to request the forced removal of a user
   from a channel.  It causes the <user> to PART from the <channel> by
   force.  For the message to be syntactically correct, there MUST be
   either one channel parameter and multiple user parameter, or as many
   channel parameters as there are user parameters.  If a "comment" is
   given, this will be sent instead of the default message, the nickname
   of the user issuing the KICK.

   The server MUST NOT send KICK messages with multiple channels or
   users to clients.  This is necessarily to maintain backward
   compatibility with old client software.
   
   Examples:

   KICK &Melbourne Matthew         ; Command to kick Matthew from
                                   &Melbourne

   KICK #Finnish John :Speaking English
                                   ; Command to kick John from #Finnish
                                   using "Speaking English" as the
                                   reason (comment).

   :WiZ!jto@tolsun.oulu.fi KICK #Finnish John
                                   ; KICK message on channel #Finnish
                                   from WiZ to remove John from channel
*/

#include "../inc/Server.hpp"

void Server::kick(Client *client, const std::vector<std::string> &tokens)
{
  if (tokens.size() < 3)
  {
    std::string msg = "\033[0;31mError: Not enough parameters for KICK command.\033[0;0m\n";
    send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
    return;
  }

  std::string channelName = tokens[1];
  std::string userName = tokens[2];
  std::string comment = (tokens.size() > 3) ? (": " + tokens[3]) : "";

  Channel *channel = findChannelByName(channelName);
  if (!channel)
  {
    std::string msg = "\033[0;31mError: No such channel.\033[0;0m\n";
    send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
    return;
  }

  if (!channel->isUserInChannel(client))
  {
    std::string msg = "\033[0;31mError: You're not in that channel.\033[0;0m\n";
    send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
    return;
  }

  if (!client->getOperator())
  {
    std::string msg = "\033[0;31mError: You're not a channel operator.\033[0;0m\n";
    send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
    return;
  }

  Client *targetClient = findClient(userName);
  if (!targetClient || !channel->isUserInChannel(targetClient))
  {
    std::string msg = "\033[0;31mError: No such user in the channel.\033[0;0m\n";
    send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
    return;
  }

  // Remove the user from the channel
  channel->removeUser(targetClient);

  // Notify the kicked user
  std::string kickMsg = "You have been kicked from " + channelName + " by " + client->getNickname() + comment + "\n";
  send(targetClient->getClientfd(), kickMsg.c_str(), kickMsg.length(), MSG_DONTROUTE);

  // Notify the client who issued the KICK command
  std::string msg = "You kicked " + userName + " from " + channelName + comment + "\n";
  send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);

  // Server log
  log.out(client->getUsername(), B);
  log.out(" kicked ");
  log.out(userName, R);
  log.out(" from ");
  log.out(channelName, Y);
  log.out(": ");
  log.nl(comment, G);
}
