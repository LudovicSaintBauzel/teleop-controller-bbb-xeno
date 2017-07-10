#ifndef SERVER_H
#define SERVER_H

#include <paddle.h>

#include <QFile>
#include <QTextStream>
#include <QDateTime>

#include <iostream>
#include <algorithm>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "defines.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <vector>
#include <QObject>
#include <mainwindow.h>

class Server : public QObject
{
    Q_OBJECT
    QThread server_thread;

public:
    Server();

    std::vector<std::string> infos_UI;

    void set_parent(void* p_parent){m_parent = p_parent;}

    bool threadServerAlive;

private:
    void* m_parent;
    struct sockaddr_in serv_addr, cli_addr;
    int sock_serv, sock_cli;

public slots:
    void run();

signals:
    void start();
    void close_gui();
    void finished();
    void load();
};

#endif // SERVER_H
