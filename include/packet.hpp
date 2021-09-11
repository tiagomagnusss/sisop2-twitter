#ifndef PACKET_HPP
#define PACKET_HPP

#include <string>

#define MAX_MESSAGE_SIZE 128

typedef enum
{
	// cliente pro server
	LOGIN,
	FOLLOW,
	SEND,
	CLIENT_HALT,

	// server pro cliente
	REPLY_LOGIN,
	REPLY_FOLLOW,
	REPLY_SEND,
	SERVER_HALT,
	//NOTIFICATION,
} EventType;

typedef struct __packet
{
	EventType type; 									//Tipo do pacote
	uint16_t seqn; 									//Número de sequência
	uint16_t length; 									//Comprimento do payload
	uint16_t timestamp; 								//Timestamp do dado
	char payload[MAX_MESSAGE_SIZE]; 				//Dados da mensagem
} packet;

packet create_packet(EventType type, uint16_t seqn, uint16_t timestamp, std::string payload);

#endif