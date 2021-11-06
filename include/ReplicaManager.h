#ifndef REPLICAMANAGER_H
#define REPLICAMANAGER_H

#define FAILURE_SOCKET_CREATION 1
#define FAILURE_RESOLVE_HOST 2
#define FAILURE_CONNECT 3

#define INVALID_CALL 4
#define INVALID_PROFILE 5
#define INVALID_SERVER 6
#define INVALID_PORT 7
#define GENERAL_ERROR 8
#define LOGIN_ERROR 9

#define DISCOVER_TIMEOUT 3

#include "Communication.h"
#include <iostream>
#include <fstream>
#include <list>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <regex>
#include <mutex>
#include "../nlohmann/json.hpp"

int discoverServers(std::pair<int, std::string> args);
void *electionThread(void* args);

int createSocket();
int connectToServer(int socketId, std::string serverAddress, std::string serverPort, bool silent);
int connectToServer(int socketId, std::string serverAddress, std::string serverPort);

class ReplicaManager
{
    private:
        std::string serverIp;
        int serverPort;

    public:
        ReplicaManager();
        ReplicaManager(int port);
        ~ReplicaManager();

        bool isPrimary();
        bool isPrimary(ReplicaManager rm);

        ReplicaManager* get_server(int port);
        void warnExiting();
        void disconnectServer(int port);
        void addLiveServer(int port, std::string address);
        void loadReplicaManagers();
        void startElection();
        void startElection(int election_port);
        void alertCoordinator(int election_port);
        void setPrimary(int election_port);
        void setIp(std::string ip);
        void setPort(int port);

        std::string getIp();
        int getPort();

        int findPrimary();
        bool electionInProgress;
};

#endif
