#ifndef SERVER_HPP
#define SERVER_HPP

#include "Packet.hpp"

#include <iostream>
#include <list>
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
#include <arpa/inet.h>

class Server
{
public:
	Server(){};

	void init();
	std::string get_addr();
	int accept_conn();
	int read_conn(int socket, packet *pkt);

	~Server()
	{
		printf("k");
		close(sockfd);
		// destroy all connections
		// save information
	};

private:
	int _create();
	int _bind();
	int _listen();

	std::string ip = "127.0.0.1";
	std::string port = "4000";
	int sockfd;
};

#endif
