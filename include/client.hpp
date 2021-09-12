#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "packet.hpp"

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

class Client {
	public:
		Client(){};

		int init_conn();
		int write_conn(packet pkt);
		int read_conn(packet* pkt);
		void close_conn();

		~Client(){
			printf("kx");
			close(sockfd);

			// destroy all connections
		};

	private:
		int _create();
		int _connect(std::string ip, std::string port);
		void _disconnect();

		int sockfd;
};

#endif
