#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include "Packet.h"
#include <iostream>
#include <unistd.h>
#include <stdio.h>

class Communication{
    private:

    public:
    static int receivePacket(int socketDescriptor, Packet *packet);
    static int sendPacket(int socketDescriptor, Packet packet);

};

#endif