/*
Command: MODE
   Parameters: <channel> *( ( "-" / "+" ) *<modes> *<modeparams> )

   The MODE command is provided so that users may query and change the
   characteristics of a channel.  For more details on available modes
   and their uses, see "Internet Relay Chat: Channel Management" [IRC-
   CHAN].  Note that there is a maximum limit of three (3) changes per
   command for modes that take a parameter.

   The available modes are as follows:
   · i: Set/remove Invite-only channel
   · t: Set/remove the restrictions of the TOPIC command to channel operators
   · k: Set/remove the channel key (password)
   · o: Give/take channel operator privilege
   · l: Set/remove the user limit to channel

   The following examples are given to help understanding the syntax of
   the MODE command, but refer to modes defined in "Internet Relay Chat:
   Channel Management" [IRC-CHAN].

   Examples:

   MODE #Finnish +o Kilroy         ; Command to give 'chanop' privileges
                                   to Kilroy on channel #Finnish.

   MODE #Finnish +v Wiz            ; Command to allow WiZ to speak on
                                   #Finnish.

   MODE #42 +k oulu                ; Command to set the channel key to
                                   "oulu".

   MODE #42 -k oulu                ; Command to remove the "oulu"
                                   channel key on channel "#42".




*/

#include "../inc/Server.hpp"

void Server::setMode(Client *client, const std::vector<std::string> &tokens)
{
  if (tokens.size() < 3)
  {
    std::string msg = "\033[0;31mError: Not enough parameters.\033[0;0m\n";
    msg.append("Usage: MODE <channel> <( \"-\" / \"+\" )modes> [parameters]\n");
    send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
    return;
  }

  if (!client->getOperator())
  {
    std::string msg = "\033[0;31mError: You're not channel operator.\033[0;0m\n";
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
    std::string msg = "\033[0;31mError: You're not on that channel.\033[0;0m\n";
    send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
    return;
  }

  std::string modes = tokens[2];
  bool adding = true;
  size_t paramIndex = 3;

  for (size_t i = 0; i < modes.size(); ++i)
  {
    char mode = modes[i];
    if (mode == '+')
    {
      adding = true;
    }
    else if (mode == '-')
    {
      adding = false;
    }
    else
    {
      switch (mode)
      {
        case 'i':
          channel->setInviteOnly(adding);
          if (adding)
          {
            std::string msg = B;
            msg.append("Channel ");
            msg.append(Y);
            msg.append(channel->getName() + " ");
            msg.append(RESET);
            msg.append("is now invite-only.\n");
            msg.append(RESET);
            send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
        
            // Server log
            log.out(client->getUsername(), B);
            log.out(" set channel ");
            log.out(channel->getName(), Y);
            log.nl(" to invite-only");
          }
          else
          {
            std::string msg = B;
            msg.append("Channel ");
            msg.append(Y);
            msg.append(channel->getName() + " ");
            msg.append(RESET);
            msg.append("is no longer invite-only.\n");
            msg.append(RESET);
            send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
        
            // Server log
            log.out(client->getUsername(), B);
            log.out(" removed invite-only from channel ");
            log.nl(channel->getName(), Y);
          }
          break;
        case 't':
          // Check if the user has the necessary permissions to change the topic restriction
          if (!client->getOperator()) {
              std::string msg = "\033[0;31mError: You do not have permission to change the topic restriction.\033[0;0m\n";
              send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
              break;
          }
      
          // Determine if we are setting (+t) or removing (-t) the topic restriction
          if (adding)
          {
            channel->setTopicRestriction(true);
            std::string msg = B;
            msg.append("Topic restriction for channel ");
            msg.append(Y);
            msg.append(channel->getName() + " ");
            msg.append(RESET);
            msg.append("has been enabled.\n");
            send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
    
            // Server log
            log.out(client->getUsername(), B);
            log.out(" enabled topic restriction for channel ");
            log.nl(channel->getName(), Y);
          }
          else
          {
            channel->setTopicRestriction(false);
            std::string msg = B;
            msg.append("Topic restriction for channel ");
            msg.append(Y);
            msg.append(channel->getName() + " ");
            msg.append(RESET);
            msg.append("has been disabled.\n");
            send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
    
            // Server log
            log.out(client->getUsername(), B);
            log.out(" disabled topic restriction for channel ");
            log.nl(channel->getName(), Y);
          }
          break;
        case 'k':
          if (paramIndex < tokens.size())
          {
            if (adding)
            {
              channel->setKey(tokens[paramIndex]);
              std::string msg = B;
              msg.append("Channel key for ");
              msg.append(Y);
              msg.append(channel->getName() + " ");
              msg.append(RESET);
              msg.append("set to ");
              msg.append(Y);
              msg.append(tokens[paramIndex] + "\n");
              msg.append(RESET);
              send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
        
              // Server log
              log.out(client->getUsername(), B);
              log.out(" set channel key for ");
              log.out(channel->getName(), Y);
              log.out(" to ");
              log.nl(tokens[paramIndex], G);
            }
            else
            {
              if (channel->getKey() == tokens[paramIndex])
              {
                channel->setKey("");
                std::string msg = B;
                msg.append("Channel key for ");
                msg.append(Y);
                msg.append(channel->getName() + " ");
                msg.append(RESET);
                msg.append("removed.\n");
                msg.append(RESET);
                send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);

                // Server log
                log.out(client->getUsername(), B);
                log.out(" removed channel key for ");
                log.nl(channel->getName(), Y);
              }
              else
              {
                std::string msg = "\033[0;31mError: Incorrect key.\033[0;0m\n";
                send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
              }
            }
            paramIndex++;
          }
          break;
        case 'o':
          if (paramIndex < tokens.size())
          {
            Client *targetClient = findClient(tokens[paramIndex]);
            if (targetClient && channel->isUserInChannel(targetClient))
            {
              if (adding)
              {
                channel->addOperator(targetClient);
                std::string msg = B;
                msg.append(targetClient->getUsername() + " ");
                msg.append(RESET);
                msg.append("is now a channel operator on ");
                msg.append(Y);
                msg.append(channel->getName() + "\n");
                msg.append(RESET);
                send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
                send(targetClient->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);

                // Server log
                log.out(client->getUsername(), B);
                log.out(" added operator: ");
                log.out(targetClient->getUsername(), B);
                log.out(" to ");
                log.nl(channel->getName(), Y);
              }
              else
              {
                channel->removeOperator(targetClient);
                std::string msg = B;
                msg.append(targetClient->getUsername() + " ");
                msg.append(RESET);
                msg.append("is no longer a channel operator on ");
                msg.append(Y);
                msg.append(channel->getName() + "\n");
                msg.append(RESET);
                send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
                send(targetClient->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);

                // Server log
                log.out(client->getUsername(), B);
                log.out(" removed operator: ");
                log.out(targetClient->getUsername(), R);
                log.out(" from ");
                log.nl(channel->getName(), Y);
              }
            }
            paramIndex++;
          }
          break;
        case 'l':
          if (adding)
          {
            if (paramIndex < tokens.size() && isNumber(tokens[paramIndex]))
            {
              size_t limit = std::strtoul(tokens[paramIndex].c_str(), NULL, 10);
              channel->setLimit(limit);
              std::string msg = B;
              msg.append(channel->getName() + " ");
              msg.append(RESET);
              msg.append("Channel limit set to ");
              msg.append(Y);
              msg.append(tokens[paramIndex] + "\n");
              msg.append(RESET);
              send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
              paramIndex++;

              // Server log
              log.out(client->getUsername(), B);
              log.out(" set channel limit: ");
              log.out(channel->getLimit(), G);
              log.out(" for ");
              log.nl(channel->getName(), Y);
            }
            else
            {
              std::string msg = "\033[0;31mError: Limit must be a whole number.\033[0;0m\n";
              send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
              return;
            }
          }
          else
          {
            channel->setLimit(0);
            std::string msg = B;
            msg.append(channel->getName() + " ");
            msg.append(RESET);
            msg.append("Channel limit removed.\n");
            msg.append(RESET);
            send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);

            // Server log
            log.out(client->getUsername(), B);
            log.out(" removed channel limit from ");
            log.nl(channel->getName(), Y);
          }
          break;
        default:
          std::string msg = "\033[0;31mError: Unknown mode.\033[0;0m\n";
          send(client->getClientfd(), msg.c_str(), msg.length(), MSG_DONTROUTE);
          break;
      }
    }
  }
}
