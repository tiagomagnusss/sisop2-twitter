#ifndef CLIENT_H
#define CLIENT_H

#define FAILURE_SOCKET_CREATION 1
#define FAILURE_RESOLVE_HOST 2
#define FAILURE_CONNECT 3

#define INVALID_CALL 4
#define INVALID_PROFILE 5
#define INVALID_SERVER 6
#define INVALID_PORT 7
#define GENERAL_ERROR 8

#include "Packet.h"
#include "Communication.h"
#include "Profile.h"
#include "ClientUI.h"
#include <iostream>
#include <list>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <regex>

class Client
{
private:

    std::string _profile;
    int _cmdSocketDescriptor;
    int _ntfSocketDescriptor;

    int createSocket();
    int connectToServer(int socketId, std::string serverAddress, std::string serverPort);

public:
    Client();
    ~Client();
    void init(std::string profile, std::string serverAddress, std::string serverPort);
    int login();
    int logoff();

    std::string get_profile();
    int get_ntf_socket();
    int get_cmd_socket();

};

#endif
