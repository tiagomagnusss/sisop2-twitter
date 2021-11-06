#include "../include/ReplicaManager.h"

static std::string DB_PATH = "database/servers.json";
std::map<int, ReplicaManager> servers;
std::map<int, ReplicaManager> liveServers;
static volatile int primaryPort = 0;
static volatile bool primaryFound = false;
std::mutex mutexElection;

int createSocket()
{
    int socketId = socket(AF_INET, SOCK_STREAM, 0);
    if (socketId < 0)
    {
        perror("An error occured creating the socket");
        exit(FAILURE_SOCKET_CREATION);
    }
    return socketId;
}

int connectToServer(int socketId, std::string serverAddress, std::string serverPort)
{
    return connectToServer(socketId, serverAddress, serverPort, false);
}

int connectToServer(int socketId, std::string serverAddress, std::string serverPort, bool silent)
{
    hostent *serverHost;
    serverHost = gethostbyname(serverAddress.c_str());

    if (serverHost == NULL)
    {
        perror("An error occured trying to resolve the server's address");
        exit(FAILURE_RESOLVE_HOST);
    }

    sockaddr_in socketAddress;
    socketAddress.sin_family = AF_INET;
    socketAddress.sin_addr = *((struct in_addr *)serverHost->h_addr);
    socketAddress.sin_port = htons(stoi(serverPort));
    bzero(&(socketAddress.sin_zero), 8);

    if (connect(socketId, (struct sockaddr *)&socketAddress, sizeof(socketAddress)) < 0)
    {
        if ( !silent )
            perror("An error occured trying to connect to the server");
        //exit(FAILURE_CONNECT);
        return -1;
    }

    return 0;
}

ReplicaManager::ReplicaManager()
{}

ReplicaManager::ReplicaManager(int port)
{
    serverPort = port;
    electionInProgress = false;
    findPrimary();

    // se não encontrou nenhum primário, se coloca como primário
    // sujeito a erro caso o primário recém tenha caído
    if ( !primaryFound )
    {
        std::cout << "Primary server not found, setting myself as primary...\n";
        setPrimary(serverPort);
    }
}

ReplicaManager::~ReplicaManager()
{}

void ReplicaManager::setIp(std::string ip)
{
    serverIp = ip;
}
void ReplicaManager::setPort(int port)
{
    serverPort = port;
}

std::string ReplicaManager::getIp()
{
    return serverIp;
}

int ReplicaManager::getPort()
{
    return serverPort;
}

bool ReplicaManager::isPrimary()
{
    return primaryPort == serverPort;
}

bool ReplicaManager::isPrimary(ReplicaManager rm)
{
    return primaryPort == rm.getPort();
}

void ReplicaManager::disconnectServer(int port)
{
    std::cout << "Removing server " << port << std::endl;
    liveServers.erase(port);

    // se o primário desconectou, verifica se deve iniciar eleição
    if ( isPrimary(servers.find(port)->second) )
    {
        std::cout << "Server was the primary, starting election...\n";
        primaryFound = false;
        primaryPort = 0;

        startElection();
    }
}

void ReplicaManager::addLiveServer(int port, std::string address)
{
    ReplicaManager rm = ReplicaManager();
    rm.setIp(address);
    rm.setPort(port);

    liveServers.insert(std::pair<int, ReplicaManager>(port, rm));
}

void ReplicaManager::loadReplicaManagers()
{
    nlohmann::json database;
    std::ifstream stream(DB_PATH);

    stream >> database;
    stream.close();

    for (auto item : database.items())
    {
        ReplicaManager rm = ReplicaManager();
        rm.setIp(item.value()["ip"].get<std::string>());
        rm.setPort(item.value()["port"].get<int>());

        servers.insert(std::pair<int, ReplicaManager>(rm.getPort(), rm));
    }
}

ReplicaManager* ReplicaManager::get_server(int port)
{
    std::cout << "Getting server " << port << std::endl;
    return &servers.at(port);
}

int ReplicaManager::findPrimary()
{
    loadReplicaManagers();

    std::list<pthread_t> threadList = std::list<pthread_t>();
    for (auto item : servers)
    {
        if ( item.first == serverPort )
            continue;

        discoverServers(std::pair<int, std::string>(item.first, item.second.getIp()));
    }

    return primaryPort;
}

void ReplicaManager::setPrimary(int election_port)
{
    mutexElection.lock();
    electionInProgress = false;
    mutexElection.unlock();

    primaryPort = election_port;
    primaryFound = true;
}

int discoverServers(std::pair<int, std::string> args)
{
    int port = args.first;
    std::string address = args.second;
    std::string payload = "" + address + ":" + std::to_string(port);

    struct timeval tv;
    tv.tv_sec = DISCOVER_TIMEOUT;
    tv.tv_usec = 0;
    int sockfd = createSocket();
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
    int connected = connectToServer(sockfd, address, std::to_string(port), true);

    if ( connected < 0 )
    {
        std::cout << "Server " << port << " is not available\n";
        return 0;
    }
    else
    {
        std::cout << "Server " << port << " is available, asking for status...\n";
        int bytesWritten = Communication::sendPacket(sockfd, createPacket(STATUS, 69, time(nullptr), payload));
        if (bytesWritten < 0)
        {
            std::cout << "An error occured trying to send the packet to the server " << port << std::endl;
            return 0;
        }

        Packet packet;
        bool serverFound = false;
        while(true)
        {
            Communication::receivePacket(sockfd, &packet, true);
            if (packet.type == COORDINATOR)
            {
                std::cout << "Server " << port << " is primary" << std::endl;
                serverFound = true;
                primaryFound = true;
                primaryPort = atoi(packet.payload);
                break;
            }
            else if ( packet.type == NOT_COORDINATOR )
            {
                std::cout << "Server " << port << " is not primary" << std::endl;
                serverFound = true;
                break;
            }
            else
            {
                std::cout << "Received a different message " << getPacketTypeName(packet.type) << std::endl;
            }
            Communication::sendPacket(sockfd, createPacket(STATUS, 69, time(nullptr), ""));
        }

        if ( serverFound )
        {
            // adiciona na lista de servidores conhecidos
            ReplicaManager rm = ReplicaManager();
            rm.setPort(port);
            rm.setIp(address);
            liveServers.insert(std::pair<int, ReplicaManager>(port, rm));
        }
    }

    Communication::sendPacket(sockfd, createPacket(LOGOFF, 69, time(nullptr), ""));
    std::cout << "Leaving status loop" << std::endl;
    close(sockfd);
    return 0;
}

void ReplicaManager::warnExiting()
{
    std::string address;
    int port;

    for (auto item : servers)
    {
        if ( item.first == getPort() )
            continue;

        address = item.second.getIp();
        port = item.first;
        int sockfd = createSocket();
        int connected = connectToServer(sockfd, address, std::to_string(port), true);

        if ( connected < 0 )
        {
            std::cout << "Server " << port << " is not available\n";
            continue;
        }
        else
        {
            std::cout << "Server " << port << " is available, sending disconnect message...\n";
            int bytesWritten = Communication::sendPacket(sockfd, createPacket(DISCONNECT, 69, time(nullptr), std::to_string(serverPort)));
            if (bytesWritten < 0)
            {
                std::cout << "An error occured trying to send the packet to the server " << port << std::endl;
                continue;
            }
        }

        close(sockfd);
    }

    std::cout << "Notified all servers, exiting...\n";
}

void ReplicaManager::startElection()
{
    startElection(serverPort);
}

void ReplicaManager::startElection(int election_port)
{
    mutexElection.lock();
    electionInProgress = true;
    mutexElection.unlock();

    primaryFound = false;
    primaryPort = 0;
    bool first_loop = true;
    bool sent = false;

    if ( liveServers.size() < 1 )
    {
        std::cout << "Only this server is available, no election needed" << std::endl;
        setPrimary(serverPort);
        return;
    }

    // verifica se pode esperar um outro servidor com prioridade iniciar
    for (auto item : liveServers)
    {
        if ( item.first < serverPort )
        {
            std::cout << "Waiting for server " << item.first << " to start election" << std::endl;
            return;
        }
    }

    while(!sent && !primaryFound)
    {
        std::cout << "Election main loop...\n";
        for ( auto item : servers )
        {
            // assim o último só vai acessar um anterior a ele se nenhum outro estiver disponível
            if ( (first_loop && item.first < serverPort) || item.first == serverPort )
            {
                continue;
            }

            std::string address = item.second.getIp();
            int port = item.first;

            int sockfd = createSocket();
            int connected = connectToServer(sockfd, address, std::to_string(port), true);

            if ( connected < 0 )
            {
                std::cout << "Server " << port << " is not available\n";
                close(sockfd);
                continue;
            }
            else
            {
                std::cout << "Server " << port << " is available, sending token " << election_port << std::endl;
                int bytesWritten = Communication::sendPacket(sockfd, createPacket(ELECTION, 69, time(nullptr), std::to_string(election_port)));

                if (bytesWritten < 0)
                {
                    std::cout << "An error occured trying to send the packet to the server " << port << std::endl;
                    close(sockfd);
                    continue;
                }

                sent = true;
                close(sockfd);
                break;
            }
        }

        first_loop = false;
    }
}

void ReplicaManager::alertCoordinator(int election_port)
{
    if ( liveServers.size() < 1 )
    {
        std::cout << "Only this server is available, there is no one to warn" << std::endl;
        setPrimary(serverPort);
        return;
    }

    for ( auto item : servers )
    {
        int port = item.first;
        std::string address = item.second.getIp();

        if ( item.first == election_port )
        {
            continue;
        }

        int sockfd = createSocket();
        int connected = connectToServer(sockfd, address, std::to_string(port), true);

        if ( connected < 0 )
        {
            std::cout << "Server " << port << " is not available\n";
        }
        else
        {
            std::cout << "Server " << port << " is available, informing that I am coordinator...\n";
            int bytesWritten = Communication::sendPacket(sockfd, createPacket(COORDINATOR, 69, time(nullptr), std::to_string(election_port)));

            if (bytesWritten < 0)
            {
                std::cout << "An error occured trying to send the packet to the server " << port << std::endl;
            }
        }

        close(sockfd);
    }
}