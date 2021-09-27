#include "../include/Server.h"

Server::Server(int port, int maxConnections)
{
    createSocket();
    bindSocket(port);
    startListening(maxConnections);
    std::cout << "Server started successfully.\n";
}

Server::~Server()
{
    close(_socketDescriptor);
}

int Server::createSocket()
{
    _socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (_socketDescriptor < 0)
    {
        perror("An error occured creating the socket");
        exit(FAILURE_SOCKET_CREATION);
    }
    return 0;
}

int Server::bindSocket(int port)
{
    sockaddr_in socketAddress;

    socketAddress.sin_family = AF_INET;
    socketAddress.sin_addr.s_addr = INADDR_ANY;
    socketAddress.sin_port = htons(port);
    bzero(&(socketAddress.sin_zero), 8);

    if (bind(_socketDescriptor, (struct sockaddr *)&socketAddress, sizeof(socketAddress)) < 0)
    {
        perror("An error occured binding the socket");
        exit(FAILURE_SOCKET_BIND);
    }

    return 0;
}

int Server::startListening(int maxConnections)
{
    if (listen(_socketDescriptor, maxConnections))
    {
        perror("An error occured trying to start listening");
        exit(FAILURE_LISTEN);
    }
    return 0;
}

int Server::acceptConnection()
{
    sockaddr_in clientSocketAddress;
    socklen_t clientSocketAddressLenght = sizeof(struct sockaddr_in);

    int newConnectionSocketDescriptor = accept(_socketDescriptor,
                                               (struct sockaddr *)&clientSocketAddress,
                                               &clientSocketAddressLenght);

    if (newConnectionSocketDescriptor < 0)
    {
        perror("An error occured trying to accept a new client");
        exit(FAILURE_ACCEPT);
    }

    return newConnectionSocketDescriptor;
}

void Server::startServing()
{
    std::list<pthread_t> threadList = std::list<pthread_t>();
    while (true)
    {
        int connectionSocketDescriptor = acceptConnection();
        std::cout << "New connection received!\n";
        pthread_t clientThread;
        pthread_create(&clientThread, NULL,
                       commandReceiverThread,
                       &connectionSocketDescriptor);
        threadList.push_back(clientThread);
    }

    for (pthread_t thread : threadList)
        pthread_join(thread, NULL);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cout << "Wrong number of arguments. Usage: ./app_server <port> <max_connections>\n";
        exit(FAILURE_LISTEN);
    }

    // Tratar erro
    int port = atoi(argv[1]);
    int maxConnections = atoi(argv[2]);

    Server server(port, maxConnections);

    server.startServing();

    return 0;
}

void *commandReceiverThread(void *args)
{
    int socketDescriptor = *((int *)args);
    int bytesRead, bytesWritten;

    Packet packet;
    Packet replyPacket;

    while (true)
    {
        bytesRead = Communication::receivePacket(socketDescriptor, &packet);

        if (bytesRead > 0)
        {
            //std::cout << "Payload size: " << packet.payloadLength << std::endl;
            std::cout << "Received package with payload: (" << getPacketTypeName(packet.type) << ") " << packet.payload << std::endl;

            if (packet.type == LOGIN)
            {
                replyPacket = createPacket(REPLY_LOGIN, 0, time(0), "Login OK!");
                std::cout << "Approved login of " << packet.payload << " on socket " << socketDescriptor << std::endl;
                Communication::sendPacket(socketDescriptor, replyPacket);
            }

            if (packet.type == LOGOFF)
            {
                std::cout << "Profile " << packet.payload << " logged off (socket " << socketDescriptor << ")" << std::endl;
            }

            if (packet.type == SEND)
            {
                replyPacket = createPacket(REPLY_SEND, 0, time(0), "Command SEND received!\n");
                std::cout << "Replying to SEND command... ";
                bytesWritten = Communication::sendPacket(socketDescriptor, replyPacket);
                if ( bytesWritten > 0 ) std::cout << "OK!" << std::endl;
                // TODO: atuar com o send
            }

            if (packet.type == FOLLOW)
            {
                replyPacket = createPacket(REPLY_FOLLOW, 0, time(0), "Command FOLLOW received!\n");
                std::cout << "Replying to FOLLOW command... ";
                bytesWritten = Communication::sendPacket(socketDescriptor, replyPacket);
                if ( bytesWritten > 0 ) std::cout << "OK!" << std::endl;
                // TODO: atuar com o follow
            }
        }
    }

    close(socketDescriptor);
    return NULL;
}