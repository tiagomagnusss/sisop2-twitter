#include "../include/ReplicaManager.h"

static std::string DB_PATH = "database/servers.json";
std::map<int, ReplicaManager> servers;
int primaryPort = 0;
static volatile bool primaryFound = false;

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
    loadReplicaManagers();
    findPrimary();
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

        discoverServer(std::pair<int, std::string>(item.first, item.second.getIp()));
    }

    return primaryPort;
}

int discoverServer(std::pair<int, std::string> args)
{
    int port = args.first;
    std::string address = args.second;

    struct timeval tv;
    tv.tv_sec = DISCOVER_TIMEOUT;
    tv.tv_usec = 0;
    int sockfd = createSocket();
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
    int connected = connectToServer(sockfd, address, std::to_string(port));

    if ( connected < 0 )
    {
        std::cout << "Server " << port << " is not available\n";
        return 0;
    }
    else
    {
        std::cout << "Server " << port << " is available, asking for status...\n";
        int bytesWritten = Communication::sendPacket(sockfd, createPacket(STATUS, 69, time(nullptr), ""));
        if (bytesWritten < 0)
        {
            std::cout << "An error occured trying to send the packet to the server " << port << std::endl;
            return 0;
        }

        Packet packet;
        while(true)
        {
            Communication::receivePacket(sockfd, &packet, true);
            if (packet.type == COORDINATOR)
            {
                std::cout << "Server " << port << " is primary" << std::endl;
                primaryFound = true;
                primaryPort = atoi(packet.payload);
                break;
            }
            else if ( packet.type == NOT_COORDINATOR )
            {
                std::cout << "Server " << port << " is not primary" << std::endl;
                break;
            }
            else
            {
                std::cout << "Received a different message " << getPacketTypeName(packet.type) << std::endl;
            }
            Communication::sendPacket(sockfd, createPacket(STATUS, 69, time(nullptr), ""));
        }
    }

    Communication::sendPacket(sockfd, createPacket(LOGOFF, 69, time(nullptr), ""));
    std::cout << "Leaving status loop" << std::endl;
    close(sockfd);
    return 0;
}