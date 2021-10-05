#ifndef CLIENTUI_H
#define CLIENTUI_H

#include <iostream>
#include <ncurses.h>
#include <string.h>
#include <list>
#include <string>
#include <algorithm> // transform
#include <regex>

#include "../include/ClientUI.h"
#include "../include/Packet.h"
#include "../include/Notification.h"


using namespace std;

class ClientUI
{
private:
    int row, col, pos, maxNotifications;
    list<Notification> notificationList;
    WINDOW *notiWnd, *notiWndBrd, *legWnd, *legWndBrd, *cmdWnd, *cmdWndBrd, *rtnWnd, *rtnWndBrd;
    WINDOW *createWindow(int height, int width, int starty, int startx, bool border = false);
    string profile;
    void printNotifications();
    void clearCommand();
    string getCommand(char command[148]);
    void clearNotifications();
    

public:
    ClientUI();
    void buildWindows();
    void setProfile(string profile);
    string waitCommand();
    void addNotification(Notification newNotification);
    void closeUI();
    void setReturn(string text);
    void setReturn(string text, char payload[MAX_MESSAGE_SIZE]);
};

#endif