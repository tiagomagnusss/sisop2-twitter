#include "../include/Packet.h"

Packet createPacket(PacketType type, uint16_t sequenceNumber, uint16_t timestamp, std::string payload)
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
