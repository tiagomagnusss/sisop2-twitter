#include "../include/Client.h"
#include <csignal>

bool interrupted = false;
char _regexProfile[20] = "@[a-zA-Z0-9_]{3,19}";
void *ntf_thread(void *args);
void *cmd_thread(void *args);

ClientUI userInterface;

std::pair<PacketType, std::string> splitMessage(std::string input);
std::string read_input();
Client cli;

void sigint_handler(int signum)
{
    interrupted = true;

    cli.logoff();
    userInterface.closeUI();

    exit(signum);
}

Client::Client()
{
}

Client::~Client()
{
    std::cout << "Destructing client..." << std::endl;
}

void Client::init(std::string profile, std::string serverAddress, std::string serverPort)
{
    _profile = profile;
    _cmdSocketDescriptor = createSocket();
    _ntfSocketDescriptor = createSocket();

    _serverAddress = serverAddress;
    _serverPort = serverPort;
}

int Client::createSocket()
{
    int socketId = socket(AF_INET, SOCK_STREAM, 0);
    if (socketId < 0)
    {
        perror("An error occured creating the socket");
        exit(FAILURE_SOCKET_CREATION);
    }
    return socketId;
}

int Client::connectToServer(int socketId, std::string serverAddress, std::string serverPort)
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
        exit(FAILURE_CONNECT);
    }

    return 0;
}

std::string Client::get_profile()
{
    return _profile;
}

int Client::get_ntf_socket()
{
    return _ntfSocketDescriptor;
}

int Client::get_cmd_socket()
{
    return _cmdSocketDescriptor;
}

int Client::login()
{
    Packet packet;
    int retValue = 0;
    int bytesWritten = 0;

    Communication::receivePacket(_ntfSocketDescriptor, &packet, true);
    connectToServer(_ntfSocketDescriptor, _serverAddress, _serverPort);
    bytesWritten = Communication::sendPacket(_ntfSocketDescriptor, createPacket(LOGIN, 0, time(0), _profile));
    if (bytesWritten < 0)
        return 1;

    // ignora as solicitações de login repetidas
    while( packet.type != ERROR && packet.type != REPLY_LOGIN )
    {
        Communication::receivePacket(_ntfSocketDescriptor, &packet, true);
        if ( packet.type == ERROR )
        {
            return 1;
        }
        else if ( packet.type == REPLY_LOGIN )
        {
            retValue++;
            break;
        }
    }

    sleep(1);
    bytesWritten = 0;
    connectToServer(_cmdSocketDescriptor, _serverAddress, _serverPort);
    Communication::receivePacket(_cmdSocketDescriptor, &packet, true);
    bytesWritten = Communication::sendPacket(_cmdSocketDescriptor, createPacket(LOGIN, 1, time(0), _profile));
    if (bytesWritten < 0)
        return -1;

    Communication::receivePacket(_cmdSocketDescriptor, &packet, true);
    if ( packet.type == REPLY_LOGIN )
    {
        retValue++;
    }

    return retValue;
}

int Client::logoff()
{
    Packet packet;

    while (true)
    {
        packet = createPacket(LOGOFF, 0, time(0), _profile);

        //std::cout << "Logging off notification thread of " << packet.payload << " ... ";
        int bytesWritten = Communication::sendPacket(_ntfSocketDescriptor, packet);
        if (bytesWritten > 0)
        {
            //std::cout << "OK!" << std::endl;
        }

        //std::cout << "Logging off command thread of " << packet.payload << " ... ";
        bytesWritten = Communication::sendPacket(_cmdSocketDescriptor, packet);
        if (bytesWritten > 0)
        {
            //std::cout << "OK!" << std::endl;
            break;
        }
    }
    return 0;
    close(_cmdSocketDescriptor);
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        fprintf(stderr, "Invalid call. Usage: ./app_client @<profile> <server> <port>\n");
        exit(INVALID_CALL);
    }

    char regexPort[] = "[0-9]{1,5}";
    std::string profile(argv[1]);
    std::string serverAddress(argv[2]);
    std::string serverPort(argv[3]);

    if (!std::regex_match(profile, std::regex(_regexProfile)))
    {
        fprintf(stderr, "Invalid profile name. It must begin with @ followed by 3-19 alphanumeric characters or underline (_).\n");
        exit(INVALID_PROFILE);
    }

    if (!std::regex_match(serverPort, std::regex(regexPort)))
    {
        fprintf(stderr, "Invald port. Must be a number between 0 and 65535.\n");
        exit(INVALID_PORT);
    }

    userInterface.setProfile(profile);
    if(userInterface.buildWindows()==false)
    {
	    cout << "Console window too small! Must be at least 100x30." << endl;
        signal(SIGINT, SIG_DFL);
	    raise(SIGINT);
    }

    cli.init(profile, serverAddress, serverPort);
    signal(SIGINT, sigint_handler);

    int loginResult = cli.login();
    if (loginResult < 0){
        fprintf(stderr, "An error has occured attempting to login.\n");
        signal(SIGINT, SIG_DFL);
	    raise(SIGINT);
    }
    else if ( loginResult < 2 )
    {
        fprintf(stderr, "Two users are already connected to this account. Please wait and try again.\n");
        exit(LOGIN_ERROR);
    }
    else
    {
        pthread_t ntf_thd, cmd_thd;

        pthread_create(&cmd_thd, NULL, cmd_thread, NULL);
        pthread_create(&ntf_thd, NULL, ntf_thread, NULL);
        pthread_join(cmd_thd, NULL);
        pthread_join(ntf_thd, NULL);
    }

    return 0;
}

void *ntf_thread(void *args)
{
    int socketId = cli.get_ntf_socket();
    Packet pkt;
    Notification notification;

    while (!interrupted)
    {
        //sleep(5);
        Communication::receivePacket(socketId, &pkt, true);

        // server encerrando a conexão
        if (pkt.type == SERVER_HALT)
        {
            interrupted = true;
        }
        else if (pkt.type == NOTIFICATION)
        {
            if (pkt.sequenceNumber == 0)
            {
                notification.timestamp = pkt.timestamp;
                notification.senderUser = pkt.payload;
            }
            else
            {
                notification.message = pkt.payload;
                userInterface.addNotification(notification);
            }
        }
    }

    return NULL;
}

void *cmd_thread(void *args)
{
    int socketId = cli.get_cmd_socket();
    Packet pkt;
    int bytesWritten;

    while (!interrupted)
    {
        // lê input do user
        std::string message = userInterface.waitCommand();
        std::pair<PacketType, std::string> result = splitMessage(message);

        if (interrupted || message == "EXIT" || message == "exit")
        {
            // encerra a thread
            raise(SIGINT);
            break;
        }

        // ignora se estiver vazio ou for um codigo invalido para o client
        if (message.empty() || result.first > 5)
            continue;

        if (result.first == ERROR)
        {
            userInterface.setReturn(result.second);
            //std::cout << result.second << std::endl;
            continue;
        }

        // faz o send
        bytesWritten = Communication::sendPacket(socketId, createPacket(result.first, 0, 1234, result.second));
        if (bytesWritten > 0)
        {
            userInterface.setReturn("Command sent! Waiting confirmation...");
            //std::cout << "OK!" << std::endl;
        }

        // espera a resposta
        //std::cout << "Sending " << getPacketTypeName(result.first) << " command... ";

        Communication::receivePacket(socketId, &pkt);

        if (pkt.type == ERROR)
        {
            userInterface.setReturn("Failed to send command: ", pkt.payload);
            //std::cout << "Failed to send command: " << pkt.payload << std::endl;
        }
        else
        {
            userInterface.setReturn("Success! ", pkt.payload);
            //std::cout << pkt.payload << std::endl;
        }
    }

    return NULL;
}

std::pair<PacketType, std::string> splitMessage(std::string input)
{
    std::pair<PacketType, std::string> result;

    std::string send = "SEND";
    std::string follow = "FOLLOW";

    if (strncmp(input.c_str(), send.c_str(), send.size()) == 0)
    {
        std::string message = input.substr(send.size() + 1);
        if (message.size() > 128)
        {
            result.first = ERROR;
            result.second = "Character count is greater than 128\n";
        }
        else
        {
            result.first = SEND;
            result.second = message;
        }
    }
    else if (strncmp(input.c_str(), follow.c_str(), follow.size()) == 0)
    {
        std::string profile = input.substr(follow.size() + 1);

        if (!std::regex_match(profile, std::regex(_regexProfile)))
        {
            result.first = ERROR;
            result.second = "Invalid profile name. It must begin with @ followed by 3-19 alphanumeric characters or underline (_).\n";
        }
        else
        {
            result.first = FOLLOW;
            result.second = input.substr(follow.size() + 1);
        }
    }
    else
    {
        result.first = ERROR;
        result.second = "Invalid command sent\n";
    }

    return result;
}

std::string read_input()
{
    std::string input;
    std::getline(std::cin, input);
    return input;
}
