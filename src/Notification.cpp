#include "../include/Notification.h"

// Identificadores únicos
std::atomic<uint32_t> NOTIFICATION_ID(0);

Notification setNotification(std::string message, std::list<std::string> pending)
{
    Notification notificationInstance;

    notificationInstance.id = NOTIFICATION_ID.fetch_add(1);
    notificationInstance.message = message;
    notificationInstance.timestamp = time(0);
    notificationInstance.pendingUsers = pending;

    return notificationInstance;
}
