#include "server.h"
#include "defines.h"
#include "mainwindow.h"


Server::Server()
{
    threadServerAlive = 1;
    std::thread server_thread (&Server::run, this);
    server_thread.detach();
}

void Server::run()
{
    int slen = sizeof(serv_addr), r=59, BUFLEN = 256;
    char buf[BUFLEN];
    socklen_t clilen;
    clilen = sizeof(cli_addr);

    bzero(&serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    inet_aton(SERVER_IP, &serv_addr.sin_addr);
    serv_addr.sin_port = htons(SERVER_PORT2);

    int optval = 1;
    sock_serv=socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(sock_serv, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
    r = bind(sock_serv, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    if (r<0)
    {
        std::cout << "ERROR ON BINDING" << std::endl;
        return;
    }
    while(threadServerAlive)
    {
        listen(sock_serv,5);
        sock_cli = accept(sock_serv, (struct sockaddr *) &cli_addr, &clilen);
        if (sock_cli < 0)
        {
            std::cerr << "ERROR ON ACCEPT" << std::endl;
            //return;
        }
        bzero(buf, BUFLEN);
        r = read(sock_cli, buf, 255);
        if (r<0)
        {
            std::cerr << "ERROR ON READ" << std::endl;
            //return;
        }
        r = write(sock_cli, "data received", 13);
        if (r<0)
        {
            std::cerr << "ERROR ON WRITE" << std::endl;
            //return;
        }
        shutdown(sock_cli, 2);

        MainWindow* mafenetre = (MainWindow*)(m_parent);

        if (atoi(buf)!=0)
        {
            //std::cout << "MODE : " << buf << "\n";
            mafenetre->mode_change(atoi(buf)-1);
        }
        else
        {
            //std::cerr << "Received : " << buf << "\n";
            std::string message(buf);
            if(message.compare("CLOSE_GUI") == 0)
            {
                this->close_gui();
                threadServerAlive = 0;
                //finished();
            }
            else if(message.compare("START") == 0)
            {
                this->start();
            }
            else
            {
                infos_UI.assign(12, "\0");
                int n;
                for(int i=0; i<12; i++)
                {
                    n = message.find("\t");
                    infos_UI[i] = message.substr(0, n);
                    //std::cout << "mess:" << message << "\n";
                    message = message.erase((size_t) 0, (size_t) n+1);
                    //std::cout << "part " << i << " : " << infos_UI[i] << "\n";
                    //std::cout << "mess:" << message << "\n";
                }
                //mafenetre->on_start();
                this->load();
            }
        }

    }
    shutdown(sock_serv, 2);
}
