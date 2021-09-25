#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "Packet.hpp"

#include <iostream>
#include <list>
#include <string>
#include <map>
#include <mutex>
#include <regex>

#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define INVALID_CALL 1
#define INVALID_PROFILE 2
#define INVALID_SERVER 3
#define INVALID_PORT 3
#define GENERAL_ERROR 4

class Client
{
public:
	Client(){};

	int init_conn(std::string server_address, std::string server_port);
	int write_conn(packet pkt);
	int read_conn(packet *pkt);
	void close_conn();

	~Client()
	{
		//printf("kx");
		close(_sockfd);

		// destroy all connections
	};

private:
	int _create();
	int _connect();
	void _disconnect();

	int _sockfd;
	std::string _server_address, _server_port;
};

#endif
