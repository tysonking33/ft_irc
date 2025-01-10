/*
Command: INVITE
   Parameters: <nickname> <channel>

   The INVITE command is used to invite a user to a channel.  The
   parameter <nickname> is the nickname of the person to be invited to
   the target channel <channel>.  There is no requirement that the
   channel the target user is being invited to must exist or be a valid
   channel.  However, if the channel exists, only members of the channel
   are allowed to invite other users.  When the channel has invite-only
   flag set, only channel operators may issue INVITE command.

   Only the user inviting and the user being invited will receive
   notification of the invitation.  Other channel members are not
   notified.  (This is unlike the MODE changes, and is occasionally the
   source of trouble for users.)

   Examples:

   :Angel!wings@irc.org INVITE Wiz #Dust

                                   ; Message to WiZ when he has been
                                   invited by user Angel to channel
                                   #Dust

   INVITE Wiz #Twilight_Zone       ; Command to invite WiZ to
                                   #Twilight_zone
*/

#include "../inc/Server.hpp"

void Server::setInvite(Client *client, const std::vector<std::string> &tokens)
{
  if (tokens.size() < 3)
  {
    std::string msg = "\033[0;31mError: Not enough parameters.\033[0;0m\n";
    msg.append("Usage: INVITE <nickname> <channel>\n");
    send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
    return;
  }

  std::string nickname = tokens[1];
  std::string channelName = tokens[2];
  Client *targetClient = findClient(nickname);
  Channel *channel = findChannelByName(channelName);

  if (!targetClient)
  {
    std::string msg = "\033[0;31mError: No such user.\033[0;0m\n";
    send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
    return;
  }

  if (!channel)
  {
    std::string msg = "\033[0;31mError: No such channel.\033[0;0m\n";
    send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
    return;
  }

  if (!channel->isUserInChannel(client))
  {
    std::string msg = "\033[0;31mError: You're not on that channel.\033[0;0m\n";
    send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
    return;
  }

  if (channel->isInviteOnly() && !client->getOperator())
  {
    std::string msg = "\033[0;31mError: You're not a channel operator.\033[0;0m\n";
    send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
    return;
  }

  channel->addUser(targetClient);

  std::string inviteMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@" + _servName + " INVITE " + targetClient->getNickname() + " " + channel->getName() + "\n";
  send(targetClient->getClientfd(), inviteMsg.c_str(), inviteMsg.length(), MSG_DONTROUTE);

  // Send channel topic
  std::string topicMsg = ":Welcome " + client->getNickname() + " to channel " + channel->getName();
  std::string topic = channel->getTopic();
  if (!topic.empty())
  {
    topicMsg.append(" :" + topic + "\n");
    send(targetClient->getClientfd(), topicMsg.c_str(), topicMsg.length(), MSG_DONTROUTE);
  }

  // Send list of users in the channel
  std::string userList = "\n--- Channel members ---\n";
  const std::set<Client*>& users = channel->getUsers();
  for (std::set<Client*>::const_iterator it = users.begin(); it != users.end(); ++it)
  {
    userList += (*it)->getNickname() + "!" + (*it)->getUsername() + "@" + _servName + "\n";
  }
  userList += "-----------------------\n";
  send(targetClient->getClientfd(), userList.c_str(), userList.length(), MSG_DONTROUTE);

  std::string msg = B;
  msg.append("You have invited ");
  msg.append(Y);
  msg.append(targetClient->getNickname() + " ");
  msg.append(RESET);
  msg.append("to ");
  msg.append(Y);
  msg.append(channel->getName() + "\n");
  msg.append(RESET);
  send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);

  // Server log
  log.out(client->getUsername(), B);
  log.out(" invited ");
  log.out(targetClient->getNickname(), G);
  log.out(" to ");
  log.nl(channel->getName(), Y);
}
