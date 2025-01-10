/*
Internet Relay Chat: Server Protocol
https://datatracker.ietf.org/doc/html/rfc2813

*/

#include "../inc/Server.hpp"

Server::Server(const std::string &name, int port, const std::string &password)
    : _servName(name), _port(port), _password(password), _shutdown(false)
{
  log.nl("Server is created", G);
  log.out("Server listening on Port: ", G);
  log.nl(_port, B);

  _serverfd = socket(AF_INET, SOCK_STREAM, 0);
  if (_serverfd <= 0)
  {
    log.err("Socket creation error");
    exit(1);
  }

  int opt = 1;
  if (setsockopt(_serverfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0)
  {
    log.err("Set socket option error");
    close(_serverfd);
    exit(2);
  }

  struct sockaddr_in serverAddr;
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(_port);
  if (inet_pton(AF_INET, IP_ADDR, &serverAddr.sin_addr) <= 0)
  {
    log.err("Invalid address / Address not supported");
    close(_serverfd);
    exit(3);
  }

  if (bind(_serverfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
  {
    log.err("Bind error");
    close(_serverfd);
    exit(4);
  }

  if (listen(_serverfd, MAX_CLIENTS) < 0)
  {
    log.err("Listen error");
    close(_serverfd);
    exit(5);
  }
}

Server::~Server()
{
  // Close the server file descriptor
  close(_serverfd);

  // Log the shutdown
  log.nl("Server has been shut down.");
}

/*
The select() function is used to monitor the server socket and client sockets for readability.
The read(), send() function is called only when select() indicates that there is data available to read on a socket.
The code handles new connections and client messages.
Non-blocking I/O operations.
*/
void Server::start()
{
  fd_set readfds;
  while (true)
  {
    FD_ZERO(&readfds);
    FD_SET(_serverfd, &readfds);
    int maxfd = _serverfd;

    if (_shutdown)
    {
      shutdownServer();
      break;
    }

    // Add client sockets to fd_set:
    if (!clientList.empty())
    {
      for (std::vector<Client *>::iterator it = clientList.begin(); it != clientList.end(); ++it)
      {
        int sd = (*it)->getClientfd();
        if (sd > 0) // Ensure valid file descriptor
        {
          FD_SET(sd, &readfds);
          if (sd > maxfd)
          {
            maxfd = sd;
          }
        }
      }
    }

    // select() system call to monitor multiple file descriptors -- poll() alternative
    int activity = select(maxfd + 1, &readfds, NULL, NULL, NULL);
    if (activity < 0)
    {
      std::cerr << "select error\n";
      continue;
    }

    // Check for incoming connection
    if (FD_ISSET(_serverfd, &readfds))
    {
      int clientfd = accept(_serverfd, (struct sockaddr *)NULL, NULL);
      if (clientfd < 0)
      {
        std::cerr << "accept error\n";
        continue;
      }
      Client *new_client = new Client(clientfd);
      clientList.push_back(new_client);

      // Server log
      log.out("New client connected: ", G);
      log.out("client_fd ", Y);
      log.nl(clientfd, B);

      // Send password prompt to new client
      std::string msg = B;
      msg.append("\nWelcome to " + _servName + "\n\n");
      msg.append(Y);
      msg.append("Please enter password to continue.\nPASS <serv_pass>\n");
      msg.append(RESET);
      send(clientfd, msg.c_str(), msg.length(), MSG_DONTROUTE);
    }
    handleClientMessages(readfds);
  }
}

/*
Neet to handle when client disconnects.
what to do with the client object?
send dosconnect message to all clients? / server log?
*/
void Server::handleClientMessages(fd_set &readfds)
{
  char message[1024];
  for (std::vector<Client *>::iterator it = clientList.begin(); it != clientList.end();)
  {
    int sd = (*it)->getClientfd();
    if (sd > 0 && FD_ISSET(sd, &readfds))
    {
      int valread = read(sd, message, 1024);
      if (valread == 0)
      {
        // Client disconnected
        close(sd);
        FD_CLR(sd, &readfds);
        it = clientList.erase(it);
        if (clientList.empty())
        {
          log.nl("No clients connected");
          break;
        }
      }
      else if (valread > 0)
      {
        message[valread] = '\0';
        (*it)->appendToBuffer(message); // Append received data to buffer

        // Process complete commands
        size_t pos;
        while ((pos = (*it)->getBuffer().find('\n')) != std::string::npos)
        {
          std::string command = (*it)->getBuffer().substr(0, pos);
          (*it)->eraseFromBuffer(0, pos + 1);
          if (!(*it)->isRegistered())
          {
            checkPassword(*it, command.c_str());
          }
          else
          {
            handleMessage(*it, command);
          }
        }
        ++it;
      }
      else
      {
        ++it;
      }
    }
    else
    {
      ++it;
    }
  }
}

void Server::handleMessage(Client *client, const std::string &message)
{
  // Check if the message is empty or contains only whitespace
  bool isWhitespace = true;
  for (size_t i = 0; i < message.length(); ++i)
  {
    if (!isspace(message[i]))
    {
      isWhitespace = false;
      break;
    }
  }

  if (message.empty() || isWhitespace)
  {
    return;
  }

  std::istringstream iss(message);
  std::vector<std::string> tokens;
  std::string token;

  // Extract the command
  iss >> token;
  tokens.push_back(token);

  // Check if the command is PRIVMSG
  if (token == "PRIVMSG")
  {
    // Extract the target
    iss >> token;
    tokens.push_back(token);

    // Extract the remaining message
    std::string remainingMessage;
    std::getline(iss, remainingMessage);
    if (!remainingMessage.empty() && remainingMessage[0] == ' ')
    {
      remainingMessage.erase(0, 1); // Remove leading space
    }
    // Handle the PRIVMSG command
    sendPrivateMessage(client, tokens, remainingMessage);
  }
  else
  {
    // Handle other commands
    while (iss >> token)
    {
      tokens.push_back(token);
    }
    execute(client, tokens, message);
  }
}
