#include "../include/server.hpp"

int Server::_create()
{
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) printf("Couldn't create socket");
	return 0;
}

// Bind server to the specified address and port
int Server::_bind()
{
	sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(std::stoi(port));
	server_address.sin_addr.s_addr = inet_addr(ip.c_str());
	bzero(&(server_address.sin_zero), 8);

	int bound = bind(sockfd, (struct sockaddr *) &server_address, sizeof(server_address));
	if (bound < 0) std::cout << "Couldn't bind server at " + get_addr() + ".";

	return 0;
}

int Server::_listen()
{
	if (listen(sockfd, 3) < 0) printf("Couldn't start to listen to new connection");
	return 0;
}

int Server::accept_conn()
{
	sockaddr_in client_address;
	socklen_t client_address_length = sizeof(struct sockaddr_in);

	int new_sockfd = accept(sockfd, (struct sockaddr *) &client_address, &client_address_length);

	if (new_sockfd < 0) printf("Couldn't accept connection from client");

	return new_sockfd;
}

int Server::read_conn(int socket, packet* pkt)
{
	bzero(pkt, sizeof(*pkt));
	int n = read(socket, pkt, sizeof(*pkt));
	if (n < 0) printf("Couldn't read packet from socket %i", socket);

	return 0;
}

std::string Server::get_addr()
{
    return ip + ":" + port;
}

void Server::init()
{
	_create();
	_bind();
	_listen();
}

void* init_reader_thread(void* args);

Server srv = Server();

int main(int argc, char *argv[])
{
	// inicializa o servidor
	std::cout << "Initializing TCP server..." << std::endl;
	srv.init();

	std::list<pthread_t> threads = std::list<pthread_t>();

	std::cout << "Server initialized at " << srv.get_addr() << std::endl;

	while(true)
	{
		// aceita as conexões
		int sockets = srv.accept_conn();

		// cria uma thread pra lidar com cada client
		pthread_t client_thread;
		pthread_create(&client_thread, NULL, init_reader_thread, &sockets);
		threads.push_back(client_thread);
	}

	// espera todas as threads terminarem
	for (pthread_t t : threads) pthread_join(t, NULL);

	std::cout << "\nExiting..." << std::endl;

	return 0;
}

void* init_reader_thread(void* args)
{
	int sockfd = *((int*) args);
	packet pkt;
	std::string x = "!";

	srv.read_conn(sockfd, &pkt);

	// TODO: verificar todos os tipos e tomar as devidas ações
	if ( pkt.type == LOGIN ) x = "L";

	std::cout << "Received package of type " << x << std::endl;

	close(sockfd);

	return NULL;
}
