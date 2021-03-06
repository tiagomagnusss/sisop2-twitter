#ifndef PACKET_H
#define PACKET_H

#include <string>
#include <string.h>
#include <ctime>

#define MAX_MESSAGE_SIZE 128

typedef enum
{
    // Client
    LOGIN,
    FOLLOW,
    SEND,
    LOGOFF,
    ERROR,

    // Server
    REQUIRE_LOGIN,
    REPLY_LOGIN,
    REPLY_FOLLOW,
    REPLY_SEND,
    SERVER_HALT,
    NOTIFICATION,
    STATUS,
    DISCONNECT,

    // Eleição
    COORDINATOR,
    NOT_COORDINATOR,
    ELECTION
} PacketType;

typedef struct _packet
{
    PacketType type;
    uint16_t sequenceNumber;
    uint16_t payloadLength;
    time_t timestamp;
    char payload[MAX_MESSAGE_SIZE];
} Packet;

Packet createPacket(PacketType type, uint16_t sequenceNumber, time_t timestamp, std::string payload);
std::string getPacketTypeName(int type);

#endif
