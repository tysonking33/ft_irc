#include "../inc/Server.hpp"

/*
Servers are uniquely identified by their name which has a maximum
   length of sixty three (63) characters.  See the protocol grammar
   rules (section 3.3.1) for what may and may not be used in a server
   name.
*/

/*
When client suspended, to resume the connection, use cmd: fg
*/
int main(int ac, char **av)
{
    if (!validateInput(ac, av))
    {
        return 1;
    }
    
    Server *server = new Server("My_IRC_Serv", atoi(av[1]), av[2]);
    //Server server("My_IRC_Serv", atoi(av[1]), av[2]);
    server->start();
    delete server;
    return 0;
}
