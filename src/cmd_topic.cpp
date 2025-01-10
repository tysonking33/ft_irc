/*
Command: TOPIC
   Parameters: <channel> [ <topic> ]

   The TOPIC command is used to change or view the topic of a channel.
   The topic for channel <channel> is returned if there is no <topic>
   given.  If the <topic> parameter is present, the topic for that
   channel will be changed, if this action is allowed for the user
   requesting it.  If the <topic> parameter is an empty string, the
   topic for that channel will be removed.

   Numeric Replies:

           ERR_NEEDMOREPARAMS              ERR_NOTONCHANNEL
           RPL_NOTOPIC                     RPL_TOPIC
           ERR_CHANOPRIVSNEEDED            ERR_NOCHANMODES

   Examples:

   :WiZ!jto@tolsun.oulu.fi TOPIC #test :New topic ; User Wiz setting the topic.

   TOPIC #test :another topic      ; Command to set the topic on #test to "another topic".

   TOPIC #test :                   ; Command to clear the topic on #test.

   TOPIC #test                     ; Command to check the topic for #test.
*/

#include "../inc/Server.hpp"

void Server::setTopic(Client *client, const std::vector<std::string> &tokens)
{
  if (tokens.size() < 2)
  {
    std::string msg = "\033[0;31mError: Not enough parameters for TOPIC command.\033[0;0m\n";
    send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
    return;
  }

  std::string channelName = tokens[1];
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

  if (tokens.size() == 2)
  {
    // View the current topic
    std::string topic = channel->getTopic();
    if (topic.empty())
    {
      std::string msg = "\033[0;32mNo topic is set for channel: \033[0;0m" + channelName + "\n";
      send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
    }
    else
    {
      std::string msg = "\033[0;32mTopic for channel \033[0;0m" + channelName + ": " + topic + "\n";
      send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
    }
    return;
  }

  // Check if the channel has topic restriction enabled and if the user is a channel operator
  if (channel->isTopicRestriction() && !client->getOperator())
  {
    std::string msg = "\033[0;31mError: You're not a channel operator.\033[0;0m\n";
    send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
    return;
  }

  std::string newTopic;
  if (tokens.size() >= 3 && tokens[2][0] == ':')
  {
    // Concatenate all parts of the topic after the colon, skipping the colon
    for (size_t i = 2; i < tokens.size(); ++i)
    {
      if (i == 2)
      {
        newTopic += tokens[i].substr(1); // Skip the colon for the first token
      }
      else
      {
        newTopic += tokens[i];
      }
      if (i != tokens.size() - 1)
      {
        newTopic += " ";
      }
    }
  }
  else
  {
    std::string msg = "\033[0;31mError: Topic must start with a colon ':'\033[0;0m\n";
    send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
    return;
  }

  channel->setTopic(newTopic);

  std::string msg;
  if (newTopic.empty())
  {
    msg = "\033[0;32mTopic for channel \033[0;0m" + channelName + " \033[0;32mhas been cleared.\033[0;0m\n";
  }
  else
  {
    msg = "\033[0;32mTopic for channel \033[0;0m" + channelName + " \033[0;32mset to: \033[0;0m" + newTopic + "\n";
  }
  send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);

  // Server log
  log.out(client->getUsername(), B);
  log.out(" set topic for channel ", G);
  log.out(channelName, Y);
  log.out(" to ", G);
  log.nl(newTopic.empty() ? "cleared" : newTopic, Y);
}
