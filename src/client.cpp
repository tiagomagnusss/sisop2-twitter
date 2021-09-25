#include "../include/client.hpp"

int Client::_create()
{
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0)
	{
		fprintf(stderr, "Erro ao criar o socket.\n");
		exit(GENERAL_ERROR);
	}
	return 0;
}

int Client::_connect()
{
	// Get server representation from hostname
	hostent *server;
	server = gethostbyname(_server_address.c_str());

	if (server == NULL)
	{
		fprintf(stderr, "Impossível resolver endereço do servidor.\n");
		exit(INVALID_SERVER);
	}

	// Set server address info
	sockaddr_in server_socket;
	server_socket.sin_family = AF_INET;
	server_socket.sin_port = htons(stoi(_server_port));
	server_socket.sin_addr = *((struct in_addr *)server->h_addr);
	bzero(&(server_socket.sin_zero), 8);

	if (connect(sockfd, (struct sockaddr *)&server_socket, sizeof(server_socket)) < 0)
	{
		fprintf(stderr, "Impossível conectar ao servidor %s:%s.\n", _server_address.c_str(), _server_port.c_str());
		exit(GENERAL_ERROR);
	}

	return 0;
}

void Client::_disconnect()
{
	close(sockfd);
}

void Client::close_conn()
{
	_disconnect();
	// avisar ao server que está desconectando
}

int Client::init_conn(std::string server_address, std::string server_port)
{
	this->_server_address = server_address;
	this->_server_port = server_port;

	_create();

	Client::_connect();

	return 0;
}

int Client::read_conn(packet *pkt)
{
	bzero(pkt, sizeof(*pkt));
	int n = read(sockfd, pkt, sizeof(*pkt));
	if (n < 0)
		printf("Couldn't read packet from socket %i", sockfd);

	return 0;
}

int Client::write_conn(packet pkt)
{
	int n = write(sockfd, &pkt, sizeof(pkt));
	if (n < 0)
		printf("Couldn't write packet to server");

	return 0;
}

std::string read_input();
Client cli = Client();

int main(int argc, char *argv[])
{
	char regex_profile[] = "@[a-zA-Z0-9_]{3,20}";
	char regex_port[] = "[0-9]{1,5}";
	std::string profile(argv[1]);
	std::string server_address(argv[2]);
	std::string server_port(argv[3]);

	if (argc != 4)
	{
		fprintf(stderr, "Chamada invalida! Modo de uso: ./app_client <perfil> <servidor> <porta>\n");
		exit(INVALID_CALL);
	}

	if (!std::regex_match(profile, std::regex(regex_profile)))
	{
		fprintf(stderr, "Nome de perfil invalido. Deve comecar com @ seguido com 3 a 19 caracteres alfanumericos ou underline (_).\n");
		exit(INVALID_PROFILE);
	}

	if (!std::regex_match(server_port, std::regex(regex_port)))
	{
		fprintf(stderr, "Porta inválida. Deve ser um número de 0-65535.\n");
		exit(INVALID_PORT);
	}

	cli.init_conn(server_address, server_port);

	printf("\nInsira algum comando: \n");

	// espera os comandos
	packet pkt;
	while (true)
	{
		// lê input do user
		std::string message = read_input();

		// se estiver vazio ignora
		if (message.empty())
			continue;

		// envia pro server
		pkt = create_packet(LOGIN, 0, 1234, message);
		std::cout << pkt.payload << std::endl;
		cli.write_conn(pkt);

		// Espera a resposta do servidor
		//cli.read_conn(&pkt);
		//std::cout << "Received reply from server: \n" << pkt.payload << std::endl;
	}

	cli.close_conn();
	return 0;
}

std::string read_input()
{
	std::string input;
	std::getline(std::cin, input);
	// verificar tamanho max

	return input;
}