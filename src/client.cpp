#include "../include/client.hpp"

int Client::_create()
{
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0) printf("Couldn't create client socket");
	return 0;
}

int Client::_connect(std::string ip, std::string port)
{
	// Get server representation from hostname
	hostent *server;
	server = gethostbyname(ip.c_str());

	if (server == NULL) printf("Couldn't get server by hostname");

	// Set server address info
	sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(stoi(port));
	server_address.sin_addr = *((struct in_addr *) server->h_addr);
	bzero(&(server_address.sin_zero), 8);

	if (connect(sockfd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
		std::cout << "Couldn't connect to the server " << ip << ":" << port;
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

int Client::init_conn()
{
	_create();

	// TODO: pegar IP e porta de um arquivo ou de argumentos do cliente
	std::string server_ip = "127.0.0.1", server_port = "4000";

	Client::_connect(server_ip, server_port);

	return 0;
}

int Client::read_conn(packet* pkt)
{
	bzero(pkt, sizeof(*pkt));
	int n = read(sockfd, pkt, sizeof(*pkt));
	if (n < 0) printf("Couldn't read packet from socket %i", sockfd);

	return 0;
}

int Client::write_conn(packet pkt)
{
	int n = write(sockfd, &pkt, sizeof(pkt));
	if (n < 0) printf("Couldn't write packet to server");

	return 0;
}

std::string read_input();
Client cli = Client();

int main(int argc, char *argv[])
{
	cli.init_conn();

	printf("\nInsira algum comando: \n");

	// espera os comandos
	packet pkt;
	while(true)
	{
		// lê input do user
		std::string message = read_input();

		// se estiver vazio ignora
		if (message.empty()) continue;

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
	std::getline (std::cin, input);
	// verificar tamanho max

	return input;
}