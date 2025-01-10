/*
Channels names are strings (beginning with a '&', '#', '+' or '!'
   character) of length up to fifty (50) characters.  Apart from the
   requirement that the first character is either '&', '#', '+' or '!',
   the only restriction on a channel name is that it SHALL NOT contain
   any spaces (' '), a control G (^G or ASCII 7), a comma (',').  Space
   is used as parameter separator and command is used as a list item
   separator by the protocol).  A colon (':') can also be used as a
   delimiter for the channel mask.  Channel names are case insensitive.

Command: JOIN
   Parameters: ( <channel> *( "," <channel> ) [ <key> *( "," <key> ) ] )
               / "0"
   The JOIN command is used by a user to request to start listening to
   the specific channel.  Servers MUST be able to parse arguments in the
   form of a list of target, but SHOULD NOT use lists when sending JOIN
   messages to clients.

   Once a user has joined a channel, he receives information about
   all commands his server receives affecting the channel.  This
   includes JOIN, MODE, KICK, PART, QUIT and of course PRIVMSG/NOTICE.
   This allows channel members to keep track of the other channel
   members, as well as channel modes.

   If a JOIN is successful, the user receives a JOIN message as
   confirmation and is then sent the channel's topic (using RPL_TOPIC) and
   the list of users who are on the channel (using RPL_NAMREPLY), which
   MUST include the user joining.

   Note that this message accepts a special argument ("0"), which is
   a special request to leave all channels the user is currently a member
   of.  The server will process this message as if the user had sent
   a PART command (See Section 3.2.2) for each channel he is a member
   of.

   Numeric Replies:

           ERR_NEEDMOREPARAMS              ERR_BANNEDFROMCHAN
           ERR_INVITEONLYCHAN              ERR_BADCHANNELKEY
           ERR_CHANNELISFULL               ERR_BADCHANMASK
           ERR_NOSUCHCHANNEL               ERR_TOOMANYCHANNELS
           ERR_TOOMANYTARGETS              ERR_UNAVAILRESOURCE
           RPL_TOPIC

   Examples:

   JOIN #foobar                    ; Command to join channel #foobar.

   JOIN &foo fubar                 ; Command to join channel &foo using
                                   key "fubar".
   JOIN #foo,&bar fubar            ; Command to join channel #foo using
                                   key "fubar" and &bar using no key.

   JOIN #foo,#bar fubar,foobar     ; Command to join channel #foo using
                                   key "fubar", and channel #bar using
                                   key "foobar".

   JOIN #foo,#bar                  ; Command to join channels #foo and
                                   #bar.

   JOIN 0                          ; Leave all currently joined
                                   channels.

   :WiZ!jto@tolsun.oulu.fi JOIN #Twilight_zone ; JOIN message from WiZ
                                   on channel #Twilight_zone
*/

#include "../inc/Server.hpp"

void Server::joinChannel(Client* client, const std::vector<std::string> &tokens)
{
  if (tokens.size() < 2)
  {
    std::string msg = "\033[0;31mError: No channel specified.\033[0;0m\n";
    send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
    return;
  }

  std::string channels = tokens[1];

  // Handle JOIN 0 command
  if (channels == "0")
  {
    leaveAllChannels(client);
    return;
  }

  std::string keys = tokens.size() > 2 ? tokens[2] : "";

  std::vector<std::string> channelList = split(channels, ',');
  std::vector<std::string> keyList = split(keys, ',');

  for (size_t i = 0; i < channelList.size(); ++i)
  {
    std::string channel = channelList[i];
    std::string key = i < keyList.size() ? keyList[i] : "";

    if (!isValidChannelName(channel))
    {
      std::string msg = R;
      msg.append("Error: Invalid channel name: ");
      msg.append(channel + "\n");
      msg.append(RESET);
      msg.append("Channel names must begin with one of '&', '#', '+', or '!', ");
      msg.append("and must not contain spaces, control G, or commas.\n");
      send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
      continue;
    }

    Channel* chan = findChannelByName(channel);
    if (!chan)
    {
      chan = createChannel(channel, key);
    }

    if (chan->isUserInChannel(client))
    {
      std::string msg = "\033[0;31mError: You are already in channel: \033[0;0m";
      msg.append(channel + "\n");
      send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
      continue;
    }

    if (chan->isInviteOnly() && !chan->isInvited(client))
    {
      std::string msg = "\033[0;31mError: Channel: \033[0;0m";
      msg.append(channel);
      msg.append("\033[0;31m is invite-only.\033[0;0m\n");
      send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
      continue;
    }

    if (chan->hasKey())
    {
      if (key.empty() || chan->getKey() != key)
      {
        std::string msg = "\033[0;31mError: Incorrect key for channel: \033[0;0m";
        msg.append(channel + "\n");
        send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
        continue;
      }
    }

    if (chan->isFull())
    {
      std::string msg = "\033[0;31mError: Channel: \033[0;0m";
      msg.append(channel);
      msg.append("\033[0;31m is full.\033[0;0m\n");
      send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
      continue;
    }

    chan->addUser(client);
    sendJoinMessage(client, chan);

    // Send channel topic
    std::string topicMsg = ":Welcome" + client->getNickname() + " to channel" + channel;
    std::string topic = chan->getTopic();
    if (!topic.empty())
    {
      topicMsg.append(" :" + topic + "\n");
      send(client->getClientfd(), topicMsg.c_str(), topicMsg.length(), MSG_DONTROUTE);
    }

    // Send list of users in the channel
    std::string userList = "\n--- Channel members ---\n";
    const std::set<Client*>& users = chan->getUsers();
    for (std::set<Client*>::const_iterator it = users.begin(); it != users.end(); ++it)
    {
      userList += (*it)->getNickname() + "!" + (*it)->getUsername() + "@" + _servName + "\n";
    }
    userList += "-----------------------\n";
    send(client->getClientfd(), userList.c_str(), userList.length(), MSG_DONTROUTE);

    // Server log
    log.out("User ", G);
    log.out(client->getUsername(), Y);
    log.out(" joined channel ", G);
    log.nl(channel, Y);
  }
}

void Server::leaveAllChannels(Client *client)
{
  for (std::vector<Channel*>::iterator it = channelList.begin(); it != channelList.end(); ++it)
  {
    Channel *chan = *it;
    if (chan->isUserInChannel(client))
    {
      chan->removeUser(client);
      sendPartMessage(client, chan);

      // Server log
      log.out("User ", G);
      log.out(client->getUsername(), Y);
      log.out(" left channel ", G);
      log.nl(chan->getName(), Y);
    }
  }
}
