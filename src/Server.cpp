#include "../include/Server.h"
#include "../include/Profile.h"

std::list<Notification> pendingNotificationsList;
std::map<std::string , int> onlineUsersMap;

bool interrupted = false;
Profile pfManager = Profile();

void sigint_handler(int signum)
{
    std::cout << "Interrupt received" << std::endl;
    interrupted = true;
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
    while (true)
    {
        if ( interrupted ) break;

        // TODO: aceitar conexão de notificação e de comandos ao mesmo tempo
        int connectionSocketDescriptor = acceptConnection();
        std::cout << "New connection received!\n";
        pthread_t clientThread;
        pthread_create(&clientThread, NULL,
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

    // Tratar erro
    int port = atoi(argv[1]);
    int maxConnections = atoi(argv[2]);

    Server server(port, maxConnections);
    // TODO: habilitar quando parar de debugar
    // como a leitura é bloqueante na thread, o processo só itera quando receber algo
    // signal(SIGINT, sigint_handler);

    pfManager.loadProfiles();
    server.startServing();

    std::cout << "Stopped serving, shutting down server..." << std::endl;
    return 0;
}

void *commandReceiverThread(void *args)
{
    int socketDescriptor = *((int *)args);
    int bytesRead, bytesWritten;

    Packet packet;
    Packet replyPacket;
    Profile pf;
    Notification ntf;

    while (true)
    {
        bytesWritten = 0;
        if ( interrupted )
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

            if (packet.type == LOGIN)
            {
                pf = pfManager.get_user(packet.payload);

                replyPacket = createPacket(REPLY_LOGIN, 0, time(0), "Login OK!");
                std::cout << "Approved login of " << packet.payload << " on socket " << socketDescriptor << std::endl;
                bytesWritten = Communication::sendPacket(socketDescriptor, replyPacket);

                onlineUsersMap.insert(std::pair<std::string, int>(pf.getUsername(), socketDescriptor));
                std::cout << "socket " << pf.getUsername() << onlineUsersMap.at(pf.getUsername()) << " online " << std::endl;
            }

            if (packet.type == LOGOFF)
            {
                // encerra a thread
                std::cout << "Profile " << packet.payload << " logged off (socket " << socketDescriptor << ")" << std::endl;

        		onlineUsersMap.erase(packet.payload);
                break;
            }

            if (packet.type == SEND)
            {
                std::string payloadExtract = (std::string) packet.payload;
                std::string usernameExtract = payloadExtract.substr(0,payloadExtract.find(':'));
                std::cout << "Profile " << usernameExtract << " sent " << packet.payload << std::endl;

                replyPacket = createPacket(REPLY_SEND, 0, time(0), "Command SEND received!\n");
                std::cout << "Replying to SEND command... ";
                bytesWritten = Communication::sendPacket(socketDescriptor, replyPacket);

                pf = pf.get_user(usernameExtract);
                ntf = setNotification(payloadExtract, pf.followers); //cria a notificação

                for (auto userToReceiveNotification : pf.followers)
                {
                    std::cout << "this is user " << userToReceiveNotification << std::endl;
                    if(onlineUsersMap.find(userToReceiveNotification) != onlineUsersMap.end())
                    {
                        Communication::sendPacket(onlineUsersMap.at(userToReceiveNotification), packet);
                        std::cout << "Sending to user... "<< userToReceiveNotification << onlineUsersMap.at(userToReceiveNotification) << std::endl;
                        ntf.pendingUsers.remove(userToReceiveNotification);
                    }
                }
		    }

            if( ntf.pendingUsers.size() > 0 )
            {
                // armazena a notificação na lista
                pendingNotificationsList.push_back(ntf);
            }

            if (packet.type == FOLLOW)
            {
                if ( pf.getUsername() == packet.payload )
                {
                    std::string msgError( "Users can't follow themselves\n" );
                    replyPacket = createPacket(ERROR, 0, time(0), msgError);
                }
                else if ( !pfManager.user_exists(packet.payload) )
                {
                    std::string msgError( "User does not exist\n" );
                    replyPacket = createPacket(ERROR, 0, time(0), msgError);
                }
                else
                {
                    replyPacket = createPacket(REPLY_FOLLOW, 0, time(0), "OK!\n");
                    pfManager.follow_user(pf.getUsername(), packet.payload);
                }

                std::string name = pf.getUsername();
                // atualiza os usuários
                pfManager.saveProfiles();
                pfManager.loadProfiles();
                pf = pfManager.get_user( name );

                bytesWritten = Communication::sendPacket(socketDescriptor, replyPacket);
            }

            if ( bytesWritten > 0 ) std::cout << "OK!" << std::endl;
        }
    }

    std::cout << "Shutting down thread on socket " << socketDescriptor << std::endl;
    close(socketDescriptor);
    return NULL;
}
