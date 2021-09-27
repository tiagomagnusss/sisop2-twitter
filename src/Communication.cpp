#include "../include/Communication.h"

int Communication::receivePacket(int socketDescriptor, Packet *packet, bool silent)
{
    bzero(packet, sizeof(*packet));
    int bytesRead = read(socketDescriptor, packet, sizeof(*packet));

    if (bytesRead < 0 && !silent)
    {
        fprintf(stderr, "An error occured while trying to read packet received from socket %i", socketDescriptor);
    }

    return bytesRead;
}

int Communication::receivePacket(int socketDescriptor, Packet *packet)
{
    return receivePacket(socketDescriptor, packet, false);
}

int Communication::sendPacket(int socketDescriptor, Packet packet)
{
    int bytesWritten = write(socketDescriptor, &packet, sizeof(packet));
    if (bytesWritten < 0)
    {
        fprintf(stderr, "An error occured while trying to write packet to socket %i", socketDescriptor);
    }
    return bytesWritten;
}