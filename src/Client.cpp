#include "../include/Client.h"
#include <csignal>

bool interrupted = false;
void* ntf_thread(void* args);
void* cmd_thread(void* args);

void sig_int_handler(int signum)
{
    std::cout << "Interrupt received" << std::endl;
    interrupted = true;
}


Client::Client()
{}

Client::~Client()
{
    std::cout << "Destructing client..." << std::endl;
    logoff();
}

void Client::init(std::string profile, std::string serverAddress, std::string serverPort)
{
    _profile = profile;
    _cmdSocketDescriptor = createSocket();
    _ntfSocketDescriptor = createSocket();

    connectToServer(_cmdSocketDescriptor, serverAddress, serverPort);
    //connectToServer(_ntfSocketDescriptor, serverAddress, serverPort);
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

    while (true)
    {
        packet = createPacket(LOGIN, 0, time(0), _profile);
        int bytesWritten = Communication::sendPacket(_cmdSocketDescriptor, packet);
        std::cout << "Logging in as " << packet.payload << " ... ";

        if (bytesWritten > 0)
        {
            std::cout << "Login packet sent." << std::endl;
            break;
        }
    }

    while (true)
    {
        int bytesRead = Communication::receivePacket(_cmdSocketDescriptor, &packet);

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
        int bytesWritten = Communication::sendPacket(_cmdSocketDescriptor, packet);
        if (bytesWritten > 0)
        {
            std::cout << "OK!" << std::endl;
            break;
        }
    }
    return 0;
    close(_cmdSocketDescriptor);
}

std::string read_input();
Client cli;

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

    cli.init(profile, serverAddress, serverPort);
    signal(SIGINT, sig_int_handler);
    cli.login();

    pthread_t ntf_thd, cmd_thd;
    pthread_create(&ntf_thd, NULL, ntf_thread, NULL);
    pthread_create(&cmd_thd, NULL, cmd_thread, NULL);

    pthread_join(cmd_thd, NULL);
    pthread_join(ntf_thd, NULL);

    return 0;
}

void* ntf_thread(void* args)
{
    int socketId = cli.get_ntf_socket();
    Packet pkt;

    while( !interrupted )
    {
        // TODO: remover o print na leitura
        // sleep pra não floodar o console por enquanto
        sleep(5);
        Communication::receivePacket(socketId, &pkt);

        // server encerrando a conexão
        if ( pkt.type == SERVER_HALT )
        {
            interrupted = true;
        }
        else if ( pkt.type == NOTIFICATION )
        {
            std::cout << pkt.payload << std::endl;
        }
    }

    return NULL;
}

void* cmd_thread(void* args)
{
    int socketId = cli.get_cmd_socket();
    Packet pkt;

    while(!interrupted)
    {
        // lê input do user
        std::string message = read_input();

        if (interrupted || message == "EXIT" || message == "exit")
        {
            // Notifica que o cliente vai desconectar
            Communication::sendPacket(socketId, createPacket(CLIENT_HALT, 0, 0, std::string()));
            break;
        }

        // ignora se estiver vazio
        if (message.empty()) continue;

        // TODO: separar em comando e msg
        // TODO: usar timestamp nos packets
        // faz o send
        pkt = createPacket(SEND, 0, 1234, message);
        Communication::sendPacket(socketId, pkt);
    }

    return NULL;
}

std::string read_input()
{
	std::string input;
	std::getline (std::cin, input);
	return input;
}