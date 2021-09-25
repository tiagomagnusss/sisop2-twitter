#include "../include/Notification.hpp"

// Identificadores únicos
std::atomic<uint32_t> NOTIFICATION_ID(0);

notification create_notification(std::string message, uint16_t pending)
{
    notification notification;

    notification.id = NOTIFICATION_ID.fetch_add(1);
    notification.message = message;
    notification.timestamp = time(0);
    notification.pending = pending;

    return notification;
}