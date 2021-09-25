#ifndef NOTIFICATION_HPP
#define NOTIFICATION_HPP

#include <string>
#include <atomic>
#include <ctime>

typedef struct __notification
{
    uint32_t id;         //Identificador da notificação (sugere-se um identificador único)
    time_t timestamp;    //Timestamp da notificação
    std::string message; //Mensagem
    uint16_t pending;    //Quantidade de leitores pendentes
} notification;

notification create_notification(std::string message, uint16_t pending);

#endif