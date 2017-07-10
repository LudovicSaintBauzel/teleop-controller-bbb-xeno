#ifndef PADDLE_H
#define PADDLE_H

#include <string>

#include <arpa/inet.h>

class Paddle
{
public:
    Paddle();

    bool connect(std::string ip, int port);
    bool disconnect();

    bool isConnected() {return m_isConnected;}
    bool isOnError() {return m_isOnError;}

    bool getPosition(float *position, float *positionVirtuelle, float *force, int MODE);

    bool sendMessage(std::string message, int port, bool istcp = false);

    void setPathPositionTo(std::string transition);

    bool getDataFile(std::string dest);

private:
    bool m_isConnected;

    bool m_isOnError;

    std::string m_ip;

    int m_socket;
    struct sockaddr_in m_si_paddle;
};

#endif // PADDLE_H
