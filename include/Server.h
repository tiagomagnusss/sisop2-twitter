#ifndef SERVER_H
#define SERVER_H

#define FAILURE_SOCKET_CREATION 1
#define FAILURE_SOCKET_BIND 2
#define FAILURE_LISTEN 3
#define FAILURE_ACCEPT 4

#include "Packet.h"
#include "Communication.h"
#include <iostream>
#include <list>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>

class Server
{
private:
    int _socketDescriptor;

    int createSocket();
    int bindSocket(int port);
    int startListening(int maxConnections);
    int acceptConnection();

public:
    Server(int port, int maxConnections);
    ~Server();
    void startServing();
};

void *commandReceiverThread(void *args);

#endif