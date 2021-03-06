#include "../include/Packet.h"

Packet createPacket(PacketType type, uint16_t sequenceNumber, time_t timestamp, std::string payload)
{
    Packet packet;
    packet.type = type;
    packet.sequenceNumber = sequenceNumber;
    packet.timestamp = timestamp;

    memset(packet.payload, 0, MAX_MESSAGE_SIZE);
    strcpy(packet.payload, payload.c_str());
    packet.payloadLength = sizeof(packet.payload);

    return packet;
}

std::string getPacketTypeName(int type)
{
    switch (type)
    {
    case LOGIN:
        return "LOGIN";
        break;
    case FOLLOW:
        return "FOLLOW";
        break;
    case SEND:
        return "SEND";
        break;
    case LOGOFF:
        return "LOGOFF";
        break;
    case REPLY_LOGIN:
        return "REPLY_LOGIN";
        break;
    case REPLY_FOLLOW:
        return "REPLY_FOLLOW";
        break;
    case REPLY_SEND:
        return "REPLY_SEND";
        break;
    case SERVER_HALT:
        return "SERVER_HALT";
        break;
    case NOTIFICATION:
        return "NOTIFICATION";
        break;
    case STATUS:
        return "STATUS";
        break;
    case DISCONNECT:
        return "DISCONNECT";
        break;
    case REQUIRE_LOGIN:
        return "REQUIRE_LOGIN";
        break;
    case COORDINATOR:
        return "COORDINATOR";
        break;
    case NOT_COORDINATOR:
        return "NOT_COORDINATOR";
        break;
    case ELECTION:
        return "ELECTION";
        break;
    default:
        return "INVALID_TYPE";
    }
}
