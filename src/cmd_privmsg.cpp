/*
Command: PRIVMSG
   Parameters: <msgtarget> <text to be sent>

   PRIVMSG is used to send private messages between users, as well as to
   send messages to channels.  <msgtarget> is usually the nickname of
   the recipient of the message, or a channel name.
*/

#include "../inc/Server.hpp"

void Server::sendPrivateMessage(Client* senderClient, const std::vector<std::string> &tokens, const std::string &message)
{
  if (tokens.size() < 2)
  {
    std::string msg = "\033[0;31mError: Not enough parameters for PRIVMSG command.\033[0;0m\n";
    send(senderClient->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
    return;
  }

  std::string msgTarget = tokens[1];

  // Check if the target is a channel
  if (msgTarget[0] == '#' || msgTarget[0] == '&' || msgTarget[0] == '+' || msgTarget[0] == '!')
  {
    Channel* targetChannel = findChannelByName(msgTarget);
    if (!targetChannel)
    {
      std::string msg = R;
      msg.append("Error: No such channel: " + msgTarget + "\n");
      msg.append(RESET);
      send(senderClient->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
      return;
    }

    if (!targetChannel->isUserInChannel(senderClient))
    {
      std::string msg = R;
      msg.append("Error: You're not in channel: " + msgTarget + "\n");
      msg.append(RESET);
      send(senderClient->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
      return;
    }

    std::string fullMessage = "From ";
    fullMessage.append(Y);
    fullMessage.append(senderClient->getUsername());
    fullMessage.append(RESET);
    fullMessage.append(" to ");
    fullMessage.append(Y);
    fullMessage.append(msgTarget);
    fullMessage.append(RESET);
    fullMessage.append(": ");
    fullMessage.append(message);
    fullMessage.append("\n");

    const std::set<Client*>& users = targetChannel->getUsers();
    for (std::set<Client*>::const_iterator it = users.begin(); it != users.end(); ++it)
    {
      if (*it != senderClient)
      {
        send((*it)->getClientfd(), fullMessage.c_str(), fullMessage.length(), MSG_DONTROUTE);
      }
    }

    // Server log
    log.out("Message sent from ", G);
    log.out(senderClient->getUsername(), Y);
    log.out(" to channel ", G);
    log.out(msgTarget, Y);
    log.out(": ", G);
    log.nl(message, B);
  }
  else
  {
    Client* targetClient = findClient(msgTarget);
    if (!targetClient)
    {
      std::string msg = R;
      msg.append("Error: No such nick/username: " + msgTarget + "\n");
      msg.append(RESET);
      send(senderClient->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
      return;
    }

    std::string fullMessage = "From ";
    fullMessage.append(Y);
    fullMessage.append(senderClient->getUsername());
    fullMessage.append(RESET);
    fullMessage.append(": ");
    fullMessage.append(message);
    fullMessage.append("\n");
    send(targetClient->getClientfd(), fullMessage.c_str(), fullMessage.length(), MSG_DONTROUTE);

    // Server log
    log.out("Message sent from ", G);
    log.out(senderClient->getUsername(), Y);
    log.out(" to ", G);
    log.out(targetClient->getUsername(), Y);
    log.out(": ", G);
    log.nl(message, B);
  }
}