#include "../include/Client.h"
#include <csignal>

bool interrupted = false;
char _regexProfile[20] = "@[a-zA-Z0-9_]{3,19}";
void* ntf_thread(void* args);
void* cmd_thread(void* args);

void sigint_handler(int signum)
{
    interrupted = true;
}

Client::Client()
{}

Client::~Client()
{
    std::cout << "Destructing client..." << std::endl;
}

void Client::init(std::string profile, std::string serverAddress, std::string serverPort)
{
    _profile = profile;
    _cmdSocketDescriptor = createSocket();
    _ntfSocketDescriptor = createSocket();

    connectToServer(_ntfSocketDescriptor, serverAddress, serverPort);
    connectToServer(_cmdSocketDescriptor, serverAddress, serverPort);
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

    while (true)
    {
        packet = createPacket(LOGIN, 0, time(0), _profile);
        int bytesWritten = Communication::sendPacket(_ntfSocketDescriptor, packet);
	
        std::cout << "Logging in as " << packet.payload << " on notification socket " << _ntfSocketDescriptor << " ... ";

        if (bytesWritten > 0)
        {
            std::cout << "Login packet sent." << std::endl;
        }

	bytesWritten = Communication::sendPacket(_cmdSocketDescriptor, createPacket(LOGIN, 1, time(0), _profile));

        std::cout << "Logging in as " << packet.payload << " on command socket " << _cmdSocketDescriptor << " ... ";

        if (bytesWritten > 0)
        {
            std::cout << "Login packet sent." << std::endl;
            break;
        }

    }

    while (true)
    {
        int bytesRead = Communication::receivePacket(_ntfSocketDescriptor, &packet);

        if (bytesRead > 0)
        {
            std::cout << "Server response for notification thread: " << packet.payload << std::endl;
        }

	bytesRead = Communication::receivePacket(_cmdSocketDescriptor, &packet);

        if (bytesRead > 0)
        {
            std::cout << "Server response for command thread: " << packet.payload << std::endl;
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

        std::cout << "Logging off notification thread of " << packet.payload << " ... ";
        int bytesWritten = Communication::sendPacket(_ntfSocketDescriptor, packet);
        if (bytesWritten > 0)
        {
            std::cout << "OK!" << std::endl;
        }

	std::cout << "Logging off command thread of " << packet.payload << " ... ";
	bytesWritten = Communication::sendPacket(_cmdSocketDescriptor, packet);
        if (bytesWritten > 0)
        {
            std::cout << "OK!" << std::endl;
            break;
        }
    }
    return 0;
    close(_cmdSocketDescriptor);
}

std::pair<PacketType, std::string> splitMessage(std::string input);
std::string read_input();
Client cli;

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

    if ( !std::regex_match(profile, std::regex(_regexProfile)) )
    {
        fprintf(stderr, "Invalid profile name. It must begin with @ followed by 3-19 alphanumeric characters or underline (_).\n");
        exit(INVALID_PROFILE);
    }

    if (!std::regex_match(serverPort, std::regex(regexPort)))
    {
        fprintf(stderr, "Invald port. Must be a number between 0 and 65535.\n");
        exit(INVALID_PORT);
    }

    // ClientUI client;
    // Envia o nome do usuario para exibir na tela
    // client.setProfile(profile);
    // Constroi as janelas ncurses
    // client.buildWindows();
    // thread que fica enviando notificacoes dummy
    // pthread_t testThread;
    // pthread_create(&testThread, NULL, ui_thread, NULL);
    // Comeca a esperar por comandos
    // client.waitCommand();
    // client.closeUI();

    cli.init(profile, serverAddress, serverPort);
    signal(SIGINT, SIG_DFL);

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
        //sleep(5);
        Communication::receivePacket(socketId, &pkt, true);

        // server encerrando a conexão
        if ( pkt.type == SERVER_HALT )
        {
            interrupted = true;
        }
        else if ( pkt.type == NOTIFICATION )
        {
	    if(pkt.sequenceNumber == 0)
        	std::cout << "<" << pkt.timestamp << "> "<< pkt.payload << ": ";
	    else
		std::cout << pkt.payload << std::endl;	
    
        }
    }

    return NULL;
}

void* cmd_thread(void* args)
{
    int socketId = cli.get_cmd_socket();
    Packet pkt;
    int bytesWritten;

    while(!interrupted)
    {
        // lê input do user
        std::string message = read_input();
        std::pair<PacketType, std::string> result = splitMessage(message);

        if (interrupted || message == "EXIT" || message == "exit")
        {
            // encerra a thread
	    cli.logoff();
	    signal(SIGINT, sigint_handler);
	    raise(SIGINT);
            break;
        }

        // ignora se estiver vazio ou for um codigo invalido para o client
        if (message.empty() || result.first > 5) continue;

        if ( result.first == ERROR )
        {
            std::cout << result.second << std::endl;
            continue;
        }

        // faz o send
        bytesWritten = Communication::sendPacket(socketId, createPacket(result.first, 0, 1234, result.second));
        if (bytesWritten > 0)
        {
            std::cout << "OK!" << std::endl;
        }

        // espera a resposta
        std::cout << "Sending " << getPacketTypeName(result.first) << " command... ";

        Communication::receivePacket(socketId, &pkt);

        if ( pkt.type == ERROR )
        {
            std::cout << "Failed to send command: " << pkt.payload << std::endl;
        }
        else
        {
            std::cout << pkt.payload << std::endl;
        }
    }

    return NULL;
}

std::pair<PacketType, std::string> splitMessage(std::string input)
{
    std::pair<PacketType, std::string> result;

    std::string send = "SEND";
    std::string follow = "FOLLOW";

    if ( strncmp( input.c_str(), send.c_str(), send.size() ) == 0 )
    {
        std::string message = input.substr( send.size() + 1 );
        if ( message.size() > 128 )
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
    else if ( strncmp(input.c_str(), follow.c_str(), follow.size() ) ==0 )
    {
        std::string profile = input.substr( follow.size() + 1 );

        if ( !std::regex_match(profile, std::regex(_regexProfile)) )
        {
            result.first = ERROR;
            result.second = "Invalid profile name. It must begin with @ followed by 3-19 alphanumeric characters or underline (_).\n";
        }
        else
        {
            result.first = FOLLOW;
            result.second = input.substr( follow.size() + 1 );
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
	std::getline (std::cin, input);
	return input;
}
