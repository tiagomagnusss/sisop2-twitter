#include "../include/Notification.h"

// Identificadores únicos
std::atomic<uint32_t> NOTIFICATION_ID(0);

Notification setNotification(std::string senderUser, time_t timestamp, std::string message)
{
    Notification notificationInstance;

    notificationInstance.senderUser = senderUser;
    notificationInstance.id = NOTIFICATION_ID.fetch_add(1);
    notificationInstance.message = message;
    notificationInstance.timestamp = timestamp;

    return notificationInstance;
}
