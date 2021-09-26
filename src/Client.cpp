#include "../include/Client.h"

Client::Client(std::string profile, std::string serverAddress, std::string serverPort)
{
    _profile = profile;
    createSocket();
    connectToServer(serverAddress, serverPort);
}

Client::~Client()
{
    close(_socketDescriptor);
}

int Client::createSocket()
{
    _socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (_socketDescriptor < 0)
    {
        perror("An error occured creating the socket");
        exit(FAILURE_SOCKET_CREATION);
    }
    return 0;
}

int Client::connectToServer(std::string serverAddress, std::string serverPort)
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

    if (connect(_socketDescriptor, (struct sockaddr *)&socketAddress, sizeof(socketAddress)) < 0)
    {
        perror("An error occured trying to connect to the server");
        exit(FAILURE_CONNECT);
    }

    return 0;
}

int Client::login()
{
    Packet packet;

    while (true)
    {
        packet = createPacket(LOGIN, 0, time(0), _profile);
        int bytesWritten = Communication::sendPacket(_socketDescriptor, packet);
        std::cout << "Logging in as " << packet.payload << " ... ";

        if (bytesWritten > 0)
        {
            std::cout << "Login packet sent." << std::endl;
            break;
        }
    }

    while (true)
    {
        int bytesRead = Communication::receivePacket(_socketDescriptor, &packet);

        if (bytesRead > 0)
        {
            std::cout << "Server response: " << packet.payload << std::endl;
            break;
        }
    }
    return 0;
}

int Client::logoff()
{
    Packet packet;

    while (true)
    {
        packet = createPacket(LOGOFF, 0, time(0), _profile);
        std::cout << "Logging off " << packet.payload << " ... ";
        int bytesWritten = Communication::sendPacket(_socketDescriptor, packet);
        if (bytesWritten > 0)
        {
            std::cout << "OK!" << std::endl;
            break;
        }
    }
    return 0;
    close(_socketDescriptor);
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        fprintf(stderr, "Invalid call. Usage: ./app_client @<profile> <server> <port>\n");
        exit(INVALID_CALL);
    }

    char regexProfile[] = "@[a-zA-Z0-9_]{3,20}";
    char regexPort[] = "[0-9]{1,5}";
    std::string profile(argv[1]);
    std::string serverAddress(argv[2]);
    std::string serverPort(argv[3]);

    if (!std::regex_match(profile, std::regex(regexProfile)))
    {
        fprintf(stderr, "Invalid profile name. It must begin with @ followed by 3-19 alphanumeric characters or underline (_).\n");
        exit(INVALID_PROFILE);
    }

    if (!std::regex_match(serverPort, std::regex(regexPort)))
    {
        fprintf(stderr, "Invald port. Must be a number between 0 and 65535.\n");
        exit(INVALID_PORT);
    }

    Client client(profile, serverAddress, serverPort);

    client.login();

    client.logoff();

    return 0;
}
