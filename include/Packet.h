#ifndef PACKET_H
#define PACKET_H

#include <string>
#include <string.h>

#define MAX_MESSAGE_SIZE 128

typedef enum
{
    // Client
    LOGIN,
    FOLLOW,
    SEND,
    CLIENT_HALT,

    // Server
    REPLY_LOGIN,
    REPLY_FOLLOW,
    REPLY_SEND,
    SERVER_HALT,
    NOTIFICATION
} PacketType;

typedef struct _packet
{
    PacketType type;
    uint16_t sequenceNumber;
    uint16_t payloadLength;
    uint16_t timestamp;
    char payload[MAX_MESSAGE_SIZE];
} Packet;

Packet createPacket(PacketType type, uint16_t sequenceNumber, uint16_t timestamp, std::string payload);

#endif