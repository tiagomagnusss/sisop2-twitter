#include "../include/Server.h"
#include "../include/Profile.h"

std::list<Notification> pendingNotificationsList;
std::map<std::string, std::pair<int, int>> onlineUsersMap;
static volatile bool interrupted = false;
ReplicaManager rm;

Profile pfManager = Profile();

void sigint_handler(int signum)
{
    std::cout << "\nInterrupt received" << std::endl;

    interrupted = true;
    pfManager.saveProfiles();
    rm.warnExiting();

    exit(signum);
}

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
    while (!interrupted)
    {
        int connectionSocketDescriptor = acceptConnection();
        std::cout << "New connection received! ID: " << connectionSocketDescriptor << std::endl;
        pthread_t clientThread;
        int threadId = pthread_create(&clientThread, NULL,
                       commandReceiverThread,
                       &connectionSocketDescriptor);
        threadList.push_back(clientThread);
    }

    std::cout << "Waiting for threads join..." << std::endl;
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

    std::signal(SIGINT, sigint_handler);

    int port = atoi(argv[1]);
    int maxConnections = atoi(argv[2]);

    Server server(port, maxConnections);
    // inicializa o replica manager com a porta fornecida
    rm = ReplicaManager(port);

    pfManager.loadProfiles();
    server.startServing();
    pfManager.saveProfiles();

    std::cout << "Stopped serving, shutting down server..." << std::endl;

    return 0;
}

void *commandReceiverThread(void *args)
{
    int socketDescriptor = *((int *)args);
    int bytesRead, bytesWritten;
    bool login_failed = false;
    bool exit_thread = false;

    Packet packet;
    Packet replyPacket;
    Profile* pf;
    Notification ntf;

    while(true)
    {
        bytesWritten = Communication::sendPacket(socketDescriptor, createPacket(REQUIRE_LOGIN, 0, 0, std::string("Requiring client login")));

        if ( bytesWritten > 0 )
        {
            bytesRead = Communication::receivePacket(socketDescriptor, &packet);

            if ( packet.sequenceNumber == 69 )
            {
                if ( packet.type == STATUS )
                {
                    if ( rm.isPrimary() )
                    {
                        replyPacket = createPacket(COORDINATOR, 69, time(0), std::to_string(rm.getPort()));
                        std::cout << "Replying that I am coordinator to socket " << socketDescriptor << std::endl;
                    }
                    else
                    {
                        replyPacket = createPacket(NOT_COORDINATOR, 69, time(0), "");
                        std::cout << "Replying that I am not coordinator to socket " << socketDescriptor << std::endl;
                    }

                    bytesWritten = Communication::sendPacket(socketDescriptor, replyPacket);

                    // split text into ip and port
                    std::string payload = packet.payload;
                    int split = payload.find(":");
                    std::string address = payload.substr(0, split);
                    int port = atoi(payload.substr(split + 1).c_str());

                    rm.addLiveServer(port, address);
                    break;
                }
                else if ( packet.type == COORDINATOR )
                {
                    std::cout << "Received coordinator packet from socket " << socketDescriptor << std::endl;
                    rm.setPrimary(atoi(packet.payload));
                }
                else if ( packet.type == ELECTION )
                {
                    int port = atoi(packet.payload);
                    int election_port = 0;
                    std::cout << "Election received from socket " << socketDescriptor << " with port " << port << std::endl;

                    // recebeu a própria porta, então se declara primário
                    if ( rm.getPort() == port )
                    {
                        std::cout << "Received own token, becoming coordinator..." << std::endl;
                        election_port = -1;
                    }
                    else if ( rm.getPort() < port )
                    {
                        std::cout << "My own token has priority over the one received\n";
                        election_port = rm.getPort();
                    }
                    else
                    {
                        election_port = port;
                    }

                    if ( election_port < 0 )
                    {
                        std::cout << "Alerting other servers that I am the new coordinator...\n";
                        rm.alertCoordinator(rm.getPort());
                    }
                    else
                    {
                        std::cout << "Forwarding election with port " << election_port << std::endl;
                        rm.startElection(election_port);
                    }
                }
                else if ( packet.type == DISCONNECT )
                {
                    std::cout << "Disconnecting client " << socketDescriptor << " at port " << packet.payload << std::endl;
                    rm.disconnectServer(atoi(packet.payload));
                }

                exit_thread = true;
                break;
            }
            else
            {
                pf = pfManager.get_user(packet.payload);
            }

            if (packet.type == LOGIN)
            {
                if (packet.sequenceNumber == 0)
                {
                    // se já tem o user online
                    if ( onlineUsersMap.find(pf->getUsername()) != onlineUsersMap.end() )
                    {
                        std::pair<int, int>* userSockets = &onlineUsersMap.at(pf->getUsername());

                        if ( userSockets->first <= 0 )
                        {
                            userSockets->first = socketDescriptor;
                        }
                        else if ( userSockets->second <= 0 )
                        {
                            userSockets->second = socketDescriptor;
                        }
                        else
                        {
                            Communication::sendPacket(socketDescriptor, createPacket(ERROR, 0, time(nullptr), "Two clients are already logged in as this user. Please wait and try again later."));
                            std::cout << "Two users are already connected. Ending socket with ID: " << socketDescriptor << std::endl;
                            login_failed = true;
                            break;
                        }
                    }
                    else
                    {
                        onlineUsersMap.insert(std::pair<std::string, std::pair<int, int>> (pf->getUsername(), std::pair<int, int>(socketDescriptor, 0)));
                    }

                    std::cout << "User " << pf->getUsername() << " on socket " << socketDescriptor << " is online and ready to receive notifications" << std::endl;

		            replyPacket = createPacket(REPLY_LOGIN, 0, time(0), "Login OK!");
                    std::cout << "Approved login of " << packet.payload << " on socket " << socketDescriptor << std::endl;
                    bytesWritten = Communication::sendPacket(socketDescriptor, replyPacket);

                    Notification notificationManager;
                    std::list<Notification> instanceForPendingNotificationsList = pendingNotificationsList;

                    for (auto currentNotification : instanceForPendingNotificationsList)
                    {
                        pendingNotificationsList.pop_front();
                        notificationManager = currentNotification;
                        for (auto currentUser : currentNotification.pendingUsers)
                        {
                            if (currentUser == pf->getUsername())
                            {
                                Communication::sendPacket(socketDescriptor, createPacket(NOTIFICATION, 0, currentNotification.timestamp, currentNotification.senderUser));
                                Communication::sendPacket(socketDescriptor, createPacket(NOTIFICATION, 1, currentNotification.timestamp, currentNotification.message));
                                std::cout << "Sending to user " << currentUser << " on socket " << socketDescriptor << " a pending notification..." << std::endl;
                                notificationManager.pendingUsers.remove(currentUser);
                            }
                        }

                        if (notificationManager.pendingUsers.size() > 0)
                        {
                            pendingNotificationsList.push_back(notificationManager);
                        }
                    }
                }
                else
                {
                    replyPacket = createPacket(REPLY_LOGIN, 0, time(0), "Login OK!");
                    std::cout << "Approved login of " << packet.payload << " on socket " << socketDescriptor << std::endl;
                    bytesWritten = Communication::sendPacket(socketDescriptor, replyPacket);
                }
                break;
            }
        }
    }

    while (!interrupted && !login_failed && !exit_thread)
    {
        bytesWritten = 0;
        bytesRead = 0;
        if (interrupted)
        {
            // Notifica que o server vai desconectar
            Communication::sendPacket(socketDescriptor, createPacket(SERVER_HALT, 0, 0, std::string()));
            break;
        }

        // LEITURA BLOQUEANTE
        // só vai permitir encerrar ou fazer outros processamentos quando receber algo
        bytesRead = Communication::receivePacket(socketDescriptor, &packet);
        if (bytesRead > 0)
        {
            //std::cout << "Payload size: " << packet.payloadLength << std::endl;
            std::cout << "Received package with payload: (" << getPacketTypeName(packet.type) << ") " << packet.payload << std::endl;

            if (packet.type == LOGOFF)
            {
                if ( packet.sequenceNumber == 69 )
                    break;

                // encerra a thread
                std::cout << "Profile " << packet.payload << " logged off (socket " << socketDescriptor << ")" << std::endl;

                if ( onlineUsersMap.find(pf->getUsername()) != onlineUsersMap.end() )
                {
                    if ( onlineUsersMap.at(pf->getUsername()).first == socketDescriptor )
                    {
                        onlineUsersMap.at(pf->getUsername()).first = 0;
                    }
                    else if ( onlineUsersMap.at(pf->getUsername()).second == socketDescriptor )
                    {
                        onlineUsersMap.at(pf->getUsername()).second = 0;
                    }

                    if (  onlineUsersMap.at(pf->getUsername()).first == 0 && onlineUsersMap.at(pf->getUsername()).second == 0 )
                    {
                        onlineUsersMap.erase(pf->getUsername());
                    }
                }

                break;
            }

            if (packet.type == SEND)
            {
                std::cout << "Profile " << pf->getUsername() << " sent: " << packet.payload << std::endl;

                replyPacket = createPacket(REPLY_SEND, 0, time(nullptr), "Command SEND received!");
                std::cout << "Replying to SEND command... \n";
                bytesWritten = Communication::sendPacket(socketDescriptor, replyPacket);

                ntf = setNotification(pf->getUsername(), time(nullptr), packet.payload); //cria a notificação
                std::list<std::string> usersToNotify = pf->followers;
                usersToNotify.push_front(pf->getUsername());

                for (auto userToReceiveNotification : usersToNotify)
                {
                    std::cout << "Notification will be sent to user " << userToReceiveNotification << std::endl;
                    if (onlineUsersMap.find(userToReceiveNotification) != onlineUsersMap.end())
                    {
                        std::pair<int, int> userSessions = onlineUsersMap.at(userToReceiveNotification);
                        if ( userSessions.first > 0 )
                        {
                            Communication::sendPacket(userSessions.first, createPacket(NOTIFICATION, 0, time(nullptr), pf->getUsername()));
                            Communication::sendPacket(userSessions.first, createPacket(NOTIFICATION, 1, time(nullptr), packet.payload));
                            std::cout << "Sending to user " << userToReceiveNotification << " on socket " << userSessions.first << " a notification..." << std::endl;
                        }

                        if ( userSessions.second > 0 )
                        {
                            Communication::sendPacket(userSessions.second, createPacket(NOTIFICATION, 0, time(nullptr), pf->getUsername()));
                            Communication::sendPacket(userSessions.second, createPacket(NOTIFICATION, 1, time(nullptr), packet.payload));
                            std::cout << "Sending to user " << userToReceiveNotification << " on socket " << userSessions.second << " a notification..." << std::endl;
                        }
                    }
                    else
                    {
                        ntf.pendingUsers.push_back(userToReceiveNotification);
                    }
                }

                if (ntf.pendingUsers.size() > 0)
                {
                    // armazena a notificação na lista de espera
                    pendingNotificationsList.push_back(ntf);
                }
            }

            if (packet.type == FOLLOW)
            {
                if (pf->getUsername() == packet.payload)
                {
                    std::string msgError("Users can't follow themselves\n");
                    replyPacket = createPacket(ERROR, 0, time(nullptr), msgError);
                }
                else if (!pfManager.user_exists(packet.payload))
                {
                    std::string msgError("User does not exist\n");
                    replyPacket = createPacket(ERROR, 0, time(nullptr), msgError);
                }
                else
                {
                    std::cout << "User " << pf->getUsername() << " is now following " << packet.payload << std::endl;
                    std::string msg = std::string("You're now following ").append(std::string(packet.payload)).append("!\n");
                    replyPacket = createPacket(REPLY_FOLLOW, 0, time(nullptr), msg);
                    pfManager.follow_user(pf->getUsername(), packet.payload);
                }

                bytesWritten = Communication::sendPacket(socketDescriptor, replyPacket);
            }

            if ( packet.type == STATUS )
            {
                replyPacket = createPacket(COORDINATOR, 69, time(nullptr), "");
                bytesWritten = Communication::sendPacket(socketDescriptor, replyPacket);
            }

            if (bytesWritten > 0)
                std::cout << "OK!" << std::endl;
        }
        else
            continue;
    }

    std::cout << "Shutting down thread on socket " << socketDescriptor << std::endl;
    close(socketDescriptor);
    return NULL;
}
