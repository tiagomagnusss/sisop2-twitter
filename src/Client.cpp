#include "../include/Client.h"

Client::Client(std::string serverAddress, std::string serverPort)
{
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

int Client::sendPacket(Packet packet)
{
    int bytesWritten = write(_socketDescriptor, &packet, sizeof(packet));
    if (bytesWritten < 0)
    {
        fprintf(stderr, "An error occured while trying to write packet to socket %i", _socketDescriptor);
    }
    return bytesWritten;
}

int main(int argc, char *argv[])
{
    char regexProfile[] = "@[a-zA-Z0-9_]{3,20}";
    char regexPort[] = "[0-9]{1,5}";
    std::string profile(argv[1]);
    std::string serverAddress(argv[2]);
    std::string serverPort(argv[3]);

    if (argc != 4)
    {
        fprintf(stderr, "Invalid call. Usage: ./app_client @<profile> <server> <port>\n");
        exit(INVALID_CALL);
    }

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

    Client client(serverAddress, serverPort);

    std::cout << ("Type a command: \n");

    // espera os comandos
    Packet packet;
    while (true)
    {
        // lê input do user
        std::string message;
        std::getline(std::cin, message);

        // se estiver vazio ignora
        if (message.empty())
            continue;

        // envia pro server
        packet = createPacket(LOGIN, 0, 1234, message);
        std::cout << packet.payload << std::endl;
        client.sendPacket(packet);

        // Espera a resposta do servidor
        //cli.read_conn(&pkt);
        //std::cout << "Received reply from server: \n" << pkt.payload << std::endl;
    }

    return 0;
}

