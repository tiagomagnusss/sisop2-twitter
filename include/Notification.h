#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#include <string>
#include <string.h>
#include <strings.h>
#include <list>
#include <atomic>
#include <ctime>

typedef struct _notification
{
    uint32_t id;         			//Identificador da notificação (sugere-se um identificador único)
    time_t timestamp;    			//Timestamp da notificação
    std::string senderUser;			//Remetente
    std::string message; 			//Mensagem
    std::list<std::string> pendingUsers;	//Destinatários pendentes
} Notification;

Notification setNotification(std::string senderUser, std::string message);

#endif
