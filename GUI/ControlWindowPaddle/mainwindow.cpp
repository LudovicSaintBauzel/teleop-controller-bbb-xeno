#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFile>
#include <QTextStream>
#include <QDateTime>

#include <iostream>
#include <algorithm>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>

#include <QMessageBox>


#define TCP_IP  "127.0.0.1"
#define TCP_PORT1 5020
#define TCP_PORT2 5022

void send_message(std::string);
void test(std::string);
int state = 0;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->pushButton_start, SIGNAL(clicked(bool)), this, SLOT(on_start()));
    ui->comboBox_feedBack->setCurrentIndex(1);
}


void MainWindow::on_start()
{
    std::string load_mess = "START";
    send_message(load_mess);
}


void MainWindow::on_pushButton_load_clicked()
{
    QString messageQ;
    //Recuperer les infos
    QString sName1 = ui->lineEdit_nameSubject1->text();
    if(sName1.isEmpty() || sName1.count(' ') == sName1.length())
    {
        QMessageBox::critical(this, "Error", "Check subject 1 name");
        return;
    }
    QString sName2 = ui->lineEdit_nameSubject2->text();
    if(sName2.isEmpty() || sName2.count(' ') == sName2.length())
    {
        QMessageBox::critical(this, "Error", "Check subject 2 name");
        return;
    }
    bool ok;
    QString sTrialNumber = ui->lineEdit_trialNumber->text();
    sTrialNumber.toInt(&ok);
    if(!ok)
    {
        QMessageBox::critical(this, "Error", "Check trial number");
        return;
    }
    QString sScenarioNumber = ui->lineEdit_scenarioNumber->text();
    sScenarioNumber.toInt(&ok);
    if(!ok)
    {
        QMessageBox::critical(this, "Error", "Check trial number");
        return;
    }
    QString sDuration = ui->lineEdit_duration->text();
    sScenarioNumber.toInt(&ok);
    if(!ok)
    {
        QMessageBox::critical(this, "Error", "Check duration");
        return;
    }

    messageQ = sName1 + "\t"
            + sName2 + "\t"
            + sScenarioNumber + "\t"
            + sTrialNumber + "\t"
            + QString::number(ui->comboBox_feedBack->currentIndex())
            + "\t" + sDuration + "\t"
            + QString::number(ui->comboBox_Expe->currentIndex()) + "\t"
            + QString::number(ui->checkBox_Path1->isChecked()) + "\t"
            + QString::number(ui->checkBox_Path2->isChecked()) + "\t"
            + QString::number(ui->checkBox_Obst1->isChecked()) + "\t"
            + QString::number(ui->checkBox_Obst2->isChecked()) + "\t"
            + QString::number(ui->checkBox_Score->isChecked()) + "\t";
    std::string message = messageQ.toStdString();

    //test(message);

    //Envoi dans la socket
    send_message(message);

//    if(state == 0)
//    {
//        ui->pushButton_start->setText("STOP");
//        state = 1;
//    }
//    else
//    {
//        ui->pushButton_start->setText("START");
//        state = 0;
//    }
    ui->pushButton_start->setEnabled(1);
}

MainWindow::~MainWindow()
{
    std::string stop_mess = "CLOSE_GUI";
    send_message(stop_mess);
    delete ui;
}

void MainWindow::on_comboBox_feedBack_currentIndexChanged(int index)
{
    //Envoi de l'index dans la socket
    std::string message = std::to_string(index+1);

    send_message(message);
}


void test(std::string message)
{

    std::cout << message.c_str() << std::endl;
}

void send_message(std::string message)
{
    int sock_sdprt1, n;
    struct sockaddr_in sock_struct_sdprt1;

    char buffer[256];

    // Creating socket structure
    bzero((char *) &sock_struct_sdprt1, sizeof(sock_struct_sdprt1));
    sock_struct_sdprt1.sin_family = AF_INET;
    inet_aton(TCP_IP, &sock_struct_sdprt1.sin_addr);
    sock_struct_sdprt1.sin_port = htons(TCP_PORT2);

    //Create socket
    sock_sdprt1 = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_sdprt1 < 0)
        std::cerr << "ERROR opening socket"<<std::endl;

    //Connect to socket
    if (connect(sock_sdprt1,(struct sockaddr *) &sock_struct_sdprt1,sizeof(sock_struct_sdprt1)) < 0)
        std::cerr << "ERROR connecting to socket"<<std::endl;

    //Create message
    bzero(buffer,256);
    strcpy(buffer, message.c_str());

    //Send message through socket
    n = send(sock_sdprt1,buffer,strlen(buffer),0);
    if (n < 0)
         std::cerr << "ERROR writing to socket"<<std::endl;
    bzero(buffer,256);
    n = recv(sock_sdprt1,buffer,255, 0);
    if (n < 0)
         std::cerr << "ERROR reading from socket"<<std::endl;

    //Close socket
    close(sock_sdprt1);

    std::cout << "Sent : " << message.c_str() << std::endl;

}



void MainWindow::on_comboBox_Expe_currentIndexChanged(int index)
{
    if (index == 0){
        ui->checkBox_Path1->setEnabled(0);
        ui->checkBox_Path2->setEnabled(0);
        ui->checkBox_Obst1->setEnabled(0);
        ui->checkBox_Obst2->setEnabled(0);
        ui->label_Path->setEnabled(0);
        ui->label_Obstacle->setEnabled(0);
        ui->lineEdit_scenarioNumber->setText("101");
        ui->checkBox_Score->setEnabled(1);
    }
    else if(index ==1){
        ui->checkBox_Path1->setEnabled(1);
        ui->checkBox_Path2->setEnabled(1);
        ui->checkBox_Obst1->setEnabled(1);
        ui->checkBox_Obst2->setEnabled(1);
        ui->label_Path->setEnabled(1);
        ui->label_Obstacle->setEnabled(1);
        ui->lineEdit_scenarioNumber->setText("1");
        ui->checkBox_Score->setEnabled(0);
        ui->checkBox_Score->setChecked(0);
    }
}
