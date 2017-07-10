#include "paddle.h"
#include "defines.h"

#include <QFile>


#include <iostream>
#include <algorithm>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

Paddle::Paddle()
{
    m_isConnected = false;

    m_isOnError = false;
}


bool Paddle::connect(std::string ip, int port)
{
    m_isOnError = false;
    if ((m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        m_isOnError = true;
        std::cerr << "cannot create socket paddle"<<std::endl;
        return false;
    }

    memset((char *) &m_si_paddle, 0, sizeof(m_si_paddle));

    m_si_paddle.sin_family = AF_INET;
    m_si_paddle.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &(m_si_paddle.sin_addr));


    //bind socket to port
    if( bind(m_socket , (struct sockaddr*)&m_si_paddle, sizeof(m_si_paddle) ) == -1)
    {
        m_isOnError = true;
        std::cerr << "cannot bind socket paddle"<<std::endl;
        return false;
    }


    m_ip = ip;

    int error_code;
    socklen_t error_code_size = sizeof(error_code);
    getsockopt(m_socket, SOL_SOCKET, SO_ERROR, &error_code, &error_code_size);
    if(error_code < 0)
    {
        m_isOnError = true;
        m_isConnected = false;
        std::cerr << "socket not connected"<<std::endl;

        return false;
    }

    m_isConnected = true;
    return true;
}

bool Paddle::disconnect()
{
    if(m_isConnected)
    {
        int ret = close(m_socket);
        if(ret == 0)
            return true;
        else return false;
    }
    return true;
}

bool Paddle::getPosition(float *position, float *positionVirtuelle, float *force, int MODE)
{
    char buff[1024];
    socklen_t lenaddr = sizeof(m_si_paddle);
    int sz = recvfrom(m_socket,buff,sizeof(buff),0,(struct sockaddr*)&m_si_paddle,&lenaddr);
    if(sz <= 0){
        return false;
    }
    else{
        std::string buffStr(buff);
        buffStr = buffStr.substr(0, buffStr.find("\n"));
        QString list = QString::fromStdString(buffStr);
        QStringList splitList = list.split("\t");
        std::string dataP = splitList[0].toStdString();
        std::string dataV = splitList[1].toStdString();
        std::string dataF = splitList[2].toStdString();

        replace(dataP.begin(), dataP.end(), '.', ',');
        replace(dataV.begin(), dataV.end(), '.', ',');
        replace(dataF.begin(), dataF.end(), '.', ',');
        float data1 = atof(dataP.c_str());
        float data2 = atof(dataV.c_str());
        float data3 = atof(dataF.c_str());

        *position = data1;
        *positionVirtuelle = data2;
        *force = data3;
        //std::cout << "data1 : " << data1 << "\t(data1-POSITION_OFFSET) : " << (data1-POSITION_OFFSET) << "\t(data1-POSITION_OFFSET)/POSITION_OFFSET : " << (data1-POSITION_OFFSET)/POSITION_OFFSET << "\t(data1-POSITION_OFFSET)/POSITION_OFFSET*SENSITIVITY : " << (data1-POSITION_OFFSET)/POSITION_OFFSET*SENSITIVITY << "\n";
        //std::cout << "position : " << *position << "\n\n";
        //std::cerr << data1 << "\t" << data2 << "\t" << data3 << std::endl;
        }
    return true;
}

bool Paddle::sendMessage(std::string message, int port, bool istcp)
{
    int socketSend;
    struct sockaddr_in si_paddleSend;

    if(istcp)
    {

    }
    else
    {
        socketSend = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    }

    if ((socketSend) < 0)
    {
        std::cerr << "cannot create socket"<<std::endl;
        return false;
    }

    memset((char *) &si_paddleSend, 0, sizeof(si_paddleSend));

    si_paddleSend.sin_family = AF_INET;
    si_paddleSend.sin_port = htons(port);
    inet_aton(TCP_IP, &si_paddleSend.sin_addr);

//    if( bind(socketSend , (struct sockaddr*)&si_paddleSend, sizeof(si_paddleSend) ) == -1)
//    {
//        std::cerr << "cannot bind socket send"<<std::endl;
//        return false;
//    }

    socklen_t lenaddr = sizeof(si_paddleSend);
    int sz = sendto(socketSend,message.c_str(),message.size(),0,(struct sockaddr*)&si_paddleSend,lenaddr);
    std::cout << "Sent to robot : " << message << std::endl;
    close(socketSend);

    if(sz < 0)
        return false;

   return true;


}

bool Paddle::getDataFile(std::string dest)
{
    std::string cmd = "scp paddle@"+m_ip+":/home/paddle/Manip/files/Paddle/data/data.txt"+" "+dest;
    int r = system(cmd.c_str());
    if(r == -1)
        return false;

    return true;

}




