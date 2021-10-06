#include "../include/ClientUI.h"
#include <unistd.h> //wait

std::mutex mu;

void *ui_thread(void *args);

WINDOW *ClientUI::createWindow(int height, int width, int starty, int startx, bool border)
{
    WINDOW *window;

    window = newwin(height, width, starty, startx);
    if (border)
        box(window, 0, 0);
    wrefresh(window);
    return window;
}

ClientUI::ClientUI()
{
    initscr();
    getmaxyx(stdscr, row, col);
    refresh();
}

void ClientUI::setProfile(string profile)
{
    this->profile = profile;
}

bool ClientUI::buildWindows()
{
    int wRow, cRow;
    if (row >= 30 && col >= 100)
    {
        wRow = max(1 * row / 12, 3);
        cRow = row - wRow;

        rtnWndBrd = createWindow(wRow, col, cRow, 0, true);
        rtnWnd = createWindow(wRow - 2, col - 2, cRow + 1, 1, false);

        wRow = max(2 * row / 12, 4);
        cRow -= wRow;

        cmdWndBrd = createWindow(wRow, col, cRow, 0, true);
        cmdWnd = createWindow(wRow - 2, col - 2, cRow + 1, 1, false);

        wRow = max(1 * row / 12, 3);
        cRow -= wRow;

        legWndBrd = createWindow(wRow, col, cRow, 0, true);
        legWnd = createWindow(wRow - 2, col - 2, cRow + 1, 1, false);

        wRow = cRow;
        maxNotifications = wRow - 2;

        notiWndBrd = createWindow(wRow, col, 0, 0, true);
        notiWnd = createWindow(wRow - 2, col - 2, 1, 1, false);

        wprintw(legWnd, "Welcome, ");
        wprintw(legWnd, profile.c_str());
        wprintw(legWnd, "! Commands: SEND <message> | FOLLOW <@profile> | CLEARFEED | EXIT");
        wrefresh(legWnd);

        clearCommand();

        wprintw(rtnWnd, "Login successuful.");
        wrefresh(rtnWnd);

	    return true;
    }
    else
    {
        return false;
    }
}

void ClientUI::clearCommand()
{
    wclear(cmdWnd);
    wmove(cmdWnd, 0, 0);
    wprintw(cmdWnd, "Type a command: ");
    wrefresh(cmdWnd);
}

void ClientUI::clearNotifications()
{
    notificationList.clear();
    wclear(notiWnd);
    wrefresh(notiWnd);
}

void ClientUI::setReturn(string text)
{
    mu.lock();
    wmove(rtnWnd, 0, 0);
    wclrtoeol(rtnWnd);
    wprintw(rtnWnd, text.c_str());
    wrefresh(rtnWnd);
    mu.unlock();
}

void ClientUI::setReturn(string text, char payload[MAX_MESSAGE_SIZE])
{
    mu.lock();
    wmove(rtnWnd, 0, 0);
    wclrtoeol(rtnWnd);
    wprintw(rtnWnd, text.c_str());
    wprintw(rtnWnd, payload);
    wrefresh(rtnWnd);
    mu.unlock();
}

string ClientUI::getCommand(char command[148])
{
    std::string cmd, message;

    int i = 0;

    while (i < 9 && command[i] != ' ' && command[i] != '\t' && command[i] != '\0')
    {
        cmd.push_back(command[i]);
        i++;
    }

    if (command[i] == '\t' || command[i] == ' ')
    {
        i++;
        while (command[i] != '\0')
        {
            message.push_back(command[i]);
            i++;
        }
    }

    transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

    if (cmd == "SEND")
    {
        setReturn("Message sent sucessfully!");

	    addNotification( setNotification( profile, message ));

        return cmd + " " + message;
    }
    else if (cmd == "FOLLOW")
    {
        char regexProfile[20] = "@[a-zA-Z0-9_]{3,20}";
        if (!std::regex_match(message, std::regex(regexProfile)))
        {
            setReturn("Invalid profile name. It must begin with @ followed by 3-19 alphanumeric characters or underline (_).");
            return "KEEPWAITING";
        }
        else
        {
            setReturn((string) "Now following " += message);
            return cmd + " " + message;
        }
    }
    else if (cmd == "CLEARFEED")
    {
        clearNotifications();
        setReturn("Feed cleared.");
        return "KEEPWAITING";
    }
    else if (cmd == "EXIT")
    {
        return "EXIT";
    }
    else
    {
        setReturn("Invalid command.");
        return "KEEPWAITING";
    }
}

string ClientUI::waitCommand()
{
    string command = "KEEPWAITING";
    while (command == "KEEPWAITING")
    {
        char str[148];
        wgetnstr(cmdWnd, str, 148);
        command = getCommand(str);
        clearCommand();
    }
    return command;
}
void ClientUI::printNotifications()
{
    wmove(notiWnd, 0, 0);
    int i = 0;

    for (Notification notification : notificationList)
    {
        if (i < maxNotifications)
        {
            std::tm *ptm = std::localtime(&notification.timestamp);
            char dateTime[20];
            std::strftime(dateTime, 20, "%d/%m/%y %H:%M", ptm);
            wclrtoeol(notiWnd);
            wprintw(notiWnd, dateTime);
            wprintw(notiWnd, " ");
            wprintw(notiWnd, notification.senderUser.c_str());
            wprintw(notiWnd, ": ");
            wprintw(notiWnd, notification.message.c_str());
            wprintw(notiWnd, "\n");
            i++;
        }
        else
            break;
    }
    wrefresh(notiWnd);
}

void ClientUI::addNotification(Notification newNotification)
{
    mu.lock();
    notificationList.push_front(newNotification);
    printNotifications();
    mu.unlock();
}

void ClientUI::closeUI()
{
    endwin();
}

// Instancia a UI
//ClientUI client;
// int main()
// {

//     // Envia o nome do usuario para exibir na tela
//     client.setProfile("@ajcosta");

//     // Constroi as janelas ncurses
//     client.buildWindows();

//     // thread que fica enviando notificacoes dummy
//     pthread_t testThread;
//     pthread_create(&testThread, NULL, cmd_thread, NULL);

//     // Comeca a esperar por comandos
//     client.waitCommand();

//     client.closeUI();
// }

// trhead dummy para ficar enviando "notificacoes"
/* void* ui_thread(void *args)
{
    list<string> lista;
    string texto = "@alexandre_da_costa: Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint.";
    string texto2 = "@alexandre_da_costa: Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. ";

    Notification testNot = setNotification(texto, lista);
    Notification testNot2 = setNotification(texto2, lista);

    for (int i = 0; i == 15; i++)
    {
        sleep(2);
        client.addNotification(testNot);
        sleep(2);
        client.addNotification(testNot2);
    }

    return NULL;
} */
