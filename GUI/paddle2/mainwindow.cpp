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

#include "defines.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <typeinfo>

#include <QMessageBox>

#define cimg_use_jpeg
#include "CImg.h"

std::mutex mtx;
std::condition_variable cv;
char ptChgMessage1[15], ptChgMessage2[15];
bool threadSendPartAlive = 0;
bool threadReceivePosAlive = 1;
bool scenarioCharged = 0;
int MODE = 1;
float FORCE_LIMITE_ROUGE = 1;
void send_part_changes();
float PATH_DURATION = 30.0;
float PATH_LENGTH = 30.*120.0;
//void Server();
Server *server = new Server;
using namespace cimg_library;
bool record_film = 0;
bool end_path_first=1;
bool display_score = 0;
CImg<unsigned char> img_obstacles;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // Server thread

    server_thread = new QThread;
    server->set_parent(this);
    server->moveToThread(server_thread);
    connect(server, SIGNAL(start()), this, SLOT(on_start()));
    //connect(server, SIGNAL(finished()), server, SLOT(deleteLater()));
    connect(server, SIGNAL(close_gui()), this, SLOT(close_GUI()));
    connect(server, SIGNAL(load()), this, SLOT(on_load()));
    server_thread->start();

    threadSendPartAlive = 1;
    std::thread SendPartChanges (send_part_changes);
    SendPartChanges.detach();

    //connect(ui->pushButton_start, SIGNAL(clicked(bool)), this, SLOT(on_start()));

    connect(&m_refreshUITimer, SIGNAL(timeout()), this, SLOT(on_refresh()));

    //connect(&m_getPositionTimer, SIGNAL(timeout()), this, SLOT(getPaddlePositions()));

    //connect(&m_checkIsAliveTimer, SIGNAL(timeout()), this, SLOT(on_alive()));

    m_pos1 = 0.0;

    m_pos2 = 0.0;

    m_disp1 = new CImgDisplay(WINDOW_WIDTH,WINDOW_LENGTH);
    m_disp1->set_title("UI v8 - Sujet1").show().flush();
    m_disp2 = new CImgDisplay(WINDOW_WIDTH,WINDOW_LENGTH);
    m_disp2->set_title("UI v8 - Sujet2").show().flush();


    CImg<unsigned char> image1("isir.jpg");
    CImg<unsigned char> image2("isir.jpg");
    m_disp1->display(image1);
    m_disp2->display(image2);
    m_disp1->move(SCREEN_POSITION_1X,SCREEN_POSITION_1Y);
    m_disp2->move(SCREEN_POSITION_2X,SCREEN_POSITION_2Y);

    // connect paddles
    m_paddleUser1.connect(UDP_IP, UDP_PORT1);
    m_paddleUser2.connect(UDP_IP, UDP_PORT2);

    //MODE = ui->comboBox_feedBack->currentIndex();

}

MainWindow::~MainWindow()
{
    delete m_disp1;
    delete m_disp2;
    delete m_imagePath1;
    delete m_imagePath2;

    threadSendPartAlive = 0;
    threadReceivePosAlive = 0;
    m_paddleUser1.sendMessage("0\0",UDP_PORT3);
    cv.notify_one();

    if(m_refreshUITimer.isActive()){
        m_checkIsAliveTimer.stop();
        m_paddleUser1.sendMessage("STOP\0",UDP_PORT3);
    }

    m_refreshUITimer.stop();
    //m_getPositionTimer.stop();

    server_thread->terminate();

    m_paddleUser1.disconnect();
    m_paddleUser2.disconnect();

    delete ui;
}




void MainWindow::on_load()
{
    CImg<unsigned char> image1("isir.jpg");
    CImg<unsigned char> image2("isir.jpg");
    m_disp1->display(image1);
    m_disp2->display(image2);
    // set start positions cursor and path on uis
    m_pos1 = 0.0;
    m_pos2 = 0.0;
    PATH_DURATION = atof(server->infos_UI[5].c_str());
    PATH_LENGTH = PATH_DURATION*VITESSE;
    m_yPath = -PATH_LENGTH;
    m_xPosCursor1 = WINDOW_WIDTH/2.0;
    m_xPosCursor2 = WINDOW_WIDTH/2.0;

    m_imagePath1 = new CImg<unsigned char>(PATH_WIDTH, PATH_LENGTH,1,3);
    m_imagePath2 = new CImg<unsigned char>(PATH_WIDTH, PATH_LENGTH,1,3);
    m_obstacle_loc = new CImg<unsigned char>(PATH_WIDTH, PATH_LENGTH,1,3);
    img_obstacles.assign(PATH_WIDTH, PATH_LENGTH,1,3);

    m_pathPos1.clear();
    m_pathPos1.assign(PATH_LENGTH,0);
    m_pathPos2.clear();
    m_pathPos2.assign(PATH_LENGTH,0);

    MODE = atoi(server->infos_UI[4].c_str());

    // send mode to robot
    m_paddleUser1.sendMessage(std::to_string(MODE),UDP_PORT3);

    // generate path
    if(atof(server->infos_UI[6].c_str()) == 0){
        if(!generatePATH(*m_imagePath1,*m_imagePath2))
        {
            std::cerr << "Failed to generate PATH" <<  std::endl;
            return;
        }
    }
    else if (atof(server->infos_UI[6].c_str()) == 1){
        if(!generatePATH_obstacle(*m_imagePath1,*m_imagePath2))
        {
            std::cerr << "Failed to generate PATH" <<  std::endl;
            return;
        }
    }
    //m_imagePath1->save_bmp("path1.bmp");
    //generateObstacle();
    scenarioCharged = 1;


    CImg<unsigned char> image1bis(WINDOW_WIDTH,WINDOW_LENGTH,1,3);
    CImg<unsigned char> image2bis(WINDOW_WIDTH,WINDOW_LENGTH,1,3);
    image1bis.fill(BLACK[0], BLACK[1], BLACK[2]);
    image2bis.fill(BLACK[0], BLACK[1], BLACK[2]);
    m_disp1->display(image1bis);
    m_disp2->display(image2bis);

    display_score = atoi(server->infos_UI[11].c_str());
    if(display_score){
        m_score_display1 = new QLabel;
        m_score_display1->setGeometry(SCREEN_POSITION_1X+WINDOW_WIDTH-SCORE_WIDTH, SCREEN_POSITION_1Y+30, SCORE_WIDTH, SCORE_HEIGTH);
        m_score_display1->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        m_score_display1->setAttribute(Qt::WA_DeleteOnClose);
        m_score_display1->setAlignment(Qt::AlignCenter);
        m_score_display1->setStyleSheet("QLabel { background-color : black; color : red;}");
        m_score_display1->setText("SCORE");
        m_score_display1->show();
        m_score_display1->raise();

        m_score_display2 = new QLabel;
        m_score_display2->setGeometry(SCREEN_POSITION_2X+WINDOW_WIDTH-SCORE_WIDTH, SCREEN_POSITION_2Y+30, SCORE_WIDTH, SCORE_HEIGTH);
        m_score_display2->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        m_score_display2->setAttribute(Qt::WA_DeleteOnClose);
        m_score_display2->setAlignment(Qt::AlignCenter);
        m_score_display2->setStyleSheet("QLabel { background-color : black; color : red;}");
        m_score_display2->setText("SCORE");
        m_score_display2->show();
        m_score_display2->raise();
    }

}


void MainWindow::on_start()
{

    if(!m_refreshUITimer.isActive())
    {
        //Check if loaded
        if(m_imagePath1->is_empty()){
            on_load();
        }

        //Centering
        this->centering(0);

        // send mode to robot
        m_paddleUser1.sendMessage(std::to_string(MODE),UDP_PORT3);
        // send start udp_ip2 udp_port3
        m_paddleUser1.sendMessage("START\0",UDP_PORT3);

        // start uis
        m_refreshUITimer.start(1000/FPS);
        m_getPositionTimer.start(1);
        //m_checkIsAliveTimer.start(1);
        threadReceivePosAlive = 1;
        std::thread GetPosition (&MainWindow::getPaddlePositions, this);
        GetPosition.detach();
        m_frameNumber = 0;
        score1=0;
        score2=0;
        end_path_first=1;

        //ui->pushButton_start->setText("STOP");
    }
    else
    {
        if(!terminateAndClear(0))
        {
            QMessageBox::critical(this, "Error", "unable to terminate correctly");
            std::cout << "Error: unable to terminate correctly" << std::endl;
        }
    }
}


void MainWindow::on_refresh()
{
    CImg<unsigned char> image1(WINDOW_WIDTH,WINDOW_LENGTH,1,3);
    CImg<unsigned char> image2(WINDOW_WIDTH,WINDOW_LENGTH,1,3);

    CImg<unsigned char> red(WINDOW_WIDTH,WINDOW_LENGTH,1,3);

    red.fill(255); // a diminuer pour diminuer intensité filtre


/*    // receive data
    if(!m_paddleUser1.getPosition(&m_pos1))
        return;

    if(!m_paddleUser2.getPosition(&m_pos2))
        return;
    // pause signal?*/

 /*   // send path position to paddles
    std::string mess1 = m_partChangesSujet1[Y_POS_CURSOR - m_yPath] + '\0';
    if(!m_paddleUser1.sendMessage(mess1, TCP_PORT3, true))
    {
        std::cout << "Error: unable to send message to paddle 1." << std::endl;
    }

    std::string mess2 = m_partChangesSujet2[Y_POS_CURSOR - m_yPath] + '\0';
    if(!m_paddleUser2.sendMessage(mess2, TCP_PORT4, true))
    {
        std::cout << "Error: unable to send message to paddle 2." << std::endl;
    }*/

    // refresh base display
    if(m_yPath <= -PATH_LENGTH+Y_POS_CURSOR || m_yPath >= 0)
    {
        image1.fill(GREY[0], GREY[1], GREY[2]);
        image2.fill(GREY[0], GREY[1], GREY[2]);
    }

    // refresh path position
    image1 = m_imagePath1->get_rows(-m_yPath, WINDOW_LENGTH-m_yPath-1);
    image2 = m_imagePath2->get_rows(-m_yPath, WINDOW_LENGTH-m_yPath-1);

    if(m_yPath >= WINDOW_LENGTH)
    {
        terminateAndClear(1);
        return;
    }

    // update cursor position
    m_xOldPosCursor1 = m_xPosCursor1;
    m_xOldPosCursor2 = m_xPosCursor2;

    // RP
    if (MODE == 2 || MODE == 4){
        float moyenne1 = (m_pos1+m_posVirt1)/2.0;
        float moyenne2 = (m_pos2+m_posVirt2)/2.0;
        m_xPosCursor1 = WINDOW_WIDTH/2 - (moyenne1 - POSITION_OFFSET)*(DROITE - MILIEU)/SENSITIVITY;
        m_xPosCursor2 = WINDOW_WIDTH/2 - (moyenne2 - POSITION_OFFSET)*(DROITE - MILIEU)/SENSITIVITY;
    }
    // ALONE
    else if(MODE == 0){
        float moyenne1 = m_pos1;
        float moyenne2 = m_pos2;
        m_xPosCursor1 = WINDOW_WIDTH/2 - (moyenne1 - POSITION_OFFSET)*(DROITE - MILIEU)/SENSITIVITY;
        m_xPosCursor2 = WINDOW_WIDTH/2 - (moyenne2 - POSITION_OFFSET)*(DROITE - MILIEU)/SENSITIVITY;
    }
    // HFOP / HFO
    else{
        float moyenne1 = (m_pos1+m_pos2)/2.0;
        float moyenne2 = moyenne1;
        m_xPosCursor1 = WINDOW_WIDTH/2 - (moyenne1 - POSITION_OFFSET)*(DROITE - MILIEU)/SENSITIVITY;
        m_xPosCursor2 = WINDOW_WIDTH/2 - (moyenne2 - POSITION_OFFSET)*(DROITE - MILIEU)/SENSITIVITY;
    }

    //std::cout << m_pos1 << "\t" << m_pos2 << "\n";


    //m_xPosCursor = m_disp1->mouse_x();

    unsigned char cursorColor1[3] = {0, 0, 0};
    unsigned char cursorColor2[3] = {0, 0, 0};
    int borne1 = 0;
    int borne2 = 20;
    int borne3 = 40;
    int borne4 = 60;
    float diff1 = 0.0, diff2 = 0.0;
    float RMS_max = pow(DROITE-MILIEU, 2);
    if(m_yPath + PATH_LENGTH >= Y_POS_CURSOR+1 && m_yPath <= Y_POS_CURSOR)
    {
        float pathPosX1 = m_pathPos1[Y_POS_CURSOR - m_yPath];

        if(pathPosX1 == GAUCHE || pathPosX1 == DROITE || pathPosX1 == 10000)
            diff1 = std::min(fabs(m_xPosCursor1 - GAUCHE), fabs(m_xPosCursor1 - DROITE));
        else
            diff1 = abs(m_xPosCursor1 - pathPosX1);

        if(diff1 >= borne1 && diff1 < borne2)
        {
            for(int i=0;i<3;i++)
                cursorColor1[i] = GREEN[i];
        }
        else if(diff1 >= borne2 && diff1 < borne3)
        {
            for(int i=0;i<3;i++)
                cursorColor1[i] = YELLOW[i];
        }
        else if(diff1 >= borne3 && diff1 < borne4)
        {
            for(int i=0;i<3;i++)
                cursorColor1[i] = ORANGE[i];
        }
        else
        {
            for(int i=0;i<3;i++)
                cursorColor1[i] = RED[i];
        }


        float pathPosX2 = m_pathPos2[Y_POS_CURSOR - m_yPath];

        if(pathPosX2 == GAUCHE || pathPosX2 == DROITE || pathPosX2 == 10000)
            diff2 = std::min(fabs(m_xPosCursor2 - GAUCHE), fabs(m_xPosCursor2 - DROITE));
        else
            diff2 = abs(m_xPosCursor2 - pathPosX2);

        if(diff2 >= borne1 && diff2 < borne2)
        {
            for(int i=0;i<3;i++)
                cursorColor2[i] = GREEN[i];
        }
        else if(diff2 >= borne2 && diff2 < borne3)
        {
            for(int i=0;i<3;i++)
                cursorColor2[i] = YELLOW[i];
        }
        else if(diff2 >= borne3 && diff2 < borne4)
        {
            for(int i=0;i<3;i++)
                cursorColor2[i] = ORANGE[i];
        }
        else
        {
            for(int i=0;i<3;i++)
                cursorColor2[i] = RED[i];
        }

        if(atoi(server->infos_UI[6].c_str()) == 1 || atoi(server->infos_UI[7].c_str()) == 1){
//            CImg<unsigned char> image("IMG_OBST.jpg");
            if(img_obstacles(m_xPosCursor1, Y_POS_CURSOR - m_yPath, 0, 0)>200 &&  img_obstacles(m_xPosCursor1, Y_POS_CURSOR - m_yPath, 0, 1)<50){
                for(int i=0;i<3;i++){
                    cursorColor1[i] = BLUE[i];
                    cursorColor2[i] = BLUE[i];
                }
            }
        }

        if ((MODE == 2 || MODE == 4) && (m_partChangesSujet1[Y_POS_CURSOR - m_yPath] != "\0" || m_partChangesSujet2[Y_POS_CURSOR - m_yPath] != "\0")){
            //std::unique_lock<std::mutex> lock(mtx);
            bzero(ptChgMessage1, sizeof(ptChgMessage1));
            bzero(ptChgMessage2, sizeof(ptChgMessage2));
            strcpy(ptChgMessage1, m_partChangesSujet1[Y_POS_CURSOR - m_yPath].c_str());
            strcpy(ptChgMessage2, m_partChangesSujet2[Y_POS_CURSOR - m_yPath].c_str());
          //  std::cout << m_partChangesSujet1[Y_POS_CURSOR - m_yPath];
            cv.notify_one();
        }
        score1 += (1.-diff1/(DROITE-MILIEU))*100./(PATH_DURATION*FPS);
        score2 += (1.-diff2/(DROITE-MILIEU))*100./(PATH_DURATION*FPS);
    }
    else{
        for(int i=0;i<3;i++){
            cursorColor1[i] = GREEN[i];
            cursorColor2[i] = GREEN[i];
        }
    }

    if(m_yPath <= Y_POS_CURSOR && display_score){
        m_score_display1->setText(QString::number((int)score1));
        m_score_display2->setText(QString::number((int)score2));
    }
    else if (m_yPath > Y_POS_CURSOR && end_path_first && display_score){
        end_path_first = 0;
        m_score_display1->setGeometry(SCREEN_POSITION_1X+WINDOW_WIDTH/2-SCORE_WIDTH/2, SCREEN_POSITION_1Y+WINDOW_LENGTH/2-SCORE_HEIGTH/2, SCORE_WIDTH, SCORE_HEIGTH);
        m_score_display2->setGeometry(SCREEN_POSITION_2X+WINDOW_WIDTH/2-SCORE_WIDTH/2, SCREEN_POSITION_2Y+WINDOW_LENGTH/2-SCORE_HEIGTH/2, SCORE_WIDTH, SCORE_HEIGTH);
        QString finalScore1 = "FINAL SCORE : " + m_score_display1->text();
        QString finalScore2 = "FINAL SCORE : " + m_score_display2->text();
        m_score_display1->setText(finalScore1);
        m_score_display2->setText(finalScore2);
    }


    image1.draw_circle(m_xPosCursor1, Y_POS_CURSOR,CURSOR_WIDTH,cursorColor1);
    image2.draw_circle(m_xPosCursor2, Y_POS_CURSOR,CURSOR_WIDTH,cursorColor2);

//    if((fabs(m_force1) + fabs(m_force2))/2 > FORCE_LIMITE_ROUGE){
//        cimg_forXY(image1,x,y)
//        {
//            image1(x,y,0) = (image1(x,y,0)+red(x,y))/2.0;
//        }
//        cimg_forXY(image2,x,y)
//        {
//            image2(x,y,0) = (image2(x,y,0)+red(x,y))/2.0;
//        }
//    }

    if(record_film){
        QString S1 = "../recording/image1_00"+QString::number(m_frameNumber)+".jpg";
        QString S2 = "../recording/image2_00"+QString::number(m_frameNumber)+".jpg";
        image1.save(S1.toStdString().c_str());
        image2.save(S2.toStdString().c_str());
        m_frameNumber++;
    }

    m_disp1->display(image1);
    m_disp2->display(image2);
    m_yPath += DEPLACEMENT;

}

void MainWindow::getPaddlePositions()
{
    while(threadReceivePosAlive){
        if(!m_paddleUser1.getPosition(&m_pos1, &m_posVirt1, &m_force1, MODE))
            return;

        if(!m_paddleUser2.getPosition(&m_pos2, &m_posVirt2, &m_force2, MODE))
            return;
        usleep(500);
    }

}


void MainWindow::on_alive()
{



}

/*void MainWindow::generateCURSOR(int x, int y, unsigned char color[3], cimg_library::CImg<unsigned char>* img)
{

}*/


bool MainWindow::generatePATH(CImg<unsigned char> &img1, CImg<unsigned char> &img2)
{

    &img1.fill(BLACK[0], BLACK[1], BLACK[2]);
    &img2.fill(BLACK[0], BLACK[1], BLACK[2]);

    int type = 0;
    int subtype = 0;
    bool scenarionumberok = true;
    //int scenario_number = ui->lineEdit_scenarioNumber->text().toInt(&scenarionumberok);
    int scenario_number = atoi(server->infos_UI[2].c_str());
    //std::cerr << scenario_number << std::endl;
    if(!scenarionumberok)
    {        
        QMessageBox::critical(this, "Error", "Invalid scenario number");
        std::cout << "Error: Check scenario number!!!" << std::endl;
        return false;
    }

    QString scenario_file_name = "../scenarios/SCENARIO_"+ QString::fromStdString(server->infos_UI[2]) +".txt";

    QFile scenarioFile(scenario_file_name);

    if(!scenarioFile.open(QFile::ReadOnly))
    {
        QMessageBox::critical(this, "Error", "Unable to open scenario file");
        std::cout << "Error: Unable to open scenario file" << std::endl;
        return false;
    }

    QTextStream fileStream(&scenarioFile);

    int yPos = PATH_LENGTH;
    int limite = PART_LENGTH_FORK;
    int partLength = 0;
    //int partWithObstacle = 0;

    m_partChangesSujet1.clear();
    m_partChangesSujet1.assign(PATH_LENGTH,"\0");
    m_partChangesSujet2.clear();
    m_partChangesSujet2.assign(PATH_LENGTH,"\0");

    //The loop is reversed to start at the bottom of the path's surface ( = at the start)
    //--------------------------------
    if(1) //if(scenario_number >= 1 && scenario_number <= 100) //scenarii 1-100 (or others) Trials type Groten
    {
        while(yPos > 0)
        {
            //std::cerr << yPos << std::endl;
            if(yPos == PATH_LENGTH) //Impose a straight line at the start of the path
            {
                type = 0;
                subtype = 0;
                partLength = PART_DURATION_START*VITESSE;
            }
            else if( yPos <= limite) //Impose a straight line at the end of the path
            {
                type = 0;
                subtype = 0;
                partLength = yPos;
            }
            else //Read the scenario file to build the path according to the predetermined sequency
            {
                QString line = fileStream.readLine();
                QStringList splitList = line.split("\t");

                type = splitList[1].toInt();
                subtype = splitList[3].toInt();

                if(type == 3)
                {
                    partLength = PART_LENGTH_FORK;
                }
                else if(type == 0 && subtype == 0)
                {
                    partLength = PART_LENGTH_CHOICE;
                }
                else if(type == 0 && subtype == 1)
                {
                    partLength = PART_LENGTH_REGRP;
                }
                else if(type == 1 || type == 2)
                {
                    partLength = PART_LENGTH_BODY;

                }
            }
            editTransition(yPos, type, subtype);

            generatePartGroten(img1, 1, partLength, yPos-partLength, type, subtype);
            generatePartGroten(img2, 2, partLength, yPos-partLength, type, subtype);

            generatePositionVectorGroten(partLength, yPos-partLength, type, subtype);

            yPos -= partLength;
        }
    }
    scenarioFile.close();

    return true;
}


void MainWindow::editTransition(int yPos, int type, int subtype)
{
    if(type == 0 && subtype == 0 && yPos == PATH_LENGTH)
    {
        for(int i=0; i<DEPLACEMENT+1; i++)
        {
            m_partChangesSujet1[yPos-i-1] = "START";
            m_partChangesSujet2[yPos-i-1] = "START";
        }
    }
    else if(type == 0 && subtype == 0 && yPos <= PART_LENGTH_FORK)
    {
        for(int i=0; i<DEPLACEMENT+1; i++)
        {
            m_partChangesSujet1[yPos-i-1] = "END";
            m_partChangesSujet2[yPos-i-1] = "END";
        }
        for(int i =0; i<DEPLACEMENT+1; i++)
        {
            m_partChangesSujet1[i] = "END_PATH";
            m_partChangesSujet2[i] = "END_PATH";
        }
    }
    else if(type == 3)
    {
        switch(subtype)
        {
        case 0:
        {
            for(int i=0; i<DEPLACEMENT+1; i++)
            {
                m_partChangesSujet2[yPos+PART_LENGTH_CHOICE-i-1] = "FORK_GAUCHE";
                m_partChangesSujet1[yPos+PART_LENGTH_CHOICE-i-1] = "FORK_GAUCHE";
            }
            break;
        }
        case 1:
        {
            for(int i=0; i<DEPLACEMENT+1; i++)
            {
                m_partChangesSujet2[yPos+PART_LENGTH_CHOICE-i-1] = "FORK_DROITE";
                m_partChangesSujet1[yPos+PART_LENGTH_CHOICE-i-1] = "FORK_DROITE";
            }
            break;
        }
        case 2:
        {
            for(int i=0; i<DEPLACEMENT+1; i++)
            {
                m_partChangesSujet2[yPos+PART_LENGTH_CHOICE-i-1] = "FORK_GAUCHE";
                m_partChangesSujet1[yPos+PART_LENGTH_CHOICE-i-1] = "FORK_MILIEU";
            }
            break;
        }
        case 3:
        {
            for(int i=0; i<DEPLACEMENT+1; i++)
            {
                m_partChangesSujet2[yPos+PART_LENGTH_CHOICE-i-1] = "FORK_DROITE";
                m_partChangesSujet1[yPos+PART_LENGTH_CHOICE-i-1] = "FORK_MILIEU";
            }
            break;
        }
        case 4:
        {
            for(int i=0; i<DEPLACEMENT+1; i++)
            {
                m_partChangesSujet2[yPos+PART_LENGTH_CHOICE-i-1] = "FORK_MILIEU";
                m_partChangesSujet1[yPos+PART_LENGTH_CHOICE-i-1] = "FORK_GAUCHE";
            }
            break;
        }
        case 5:
        {
            for(int i=0; i<DEPLACEMENT+1; i++)
            {
                m_partChangesSujet2[yPos+PART_LENGTH_CHOICE-i-1] = "FORK_MILIEU";
                m_partChangesSujet1[yPos+PART_LENGTH_CHOICE-i-1] = "FORK_DROITE";
            }
            break;
        }
        case 6:
        {
            for(int i=0; i<DEPLACEMENT+1; i++)
            {
                m_partChangesSujet2[yPos+PART_LENGTH_CHOICE-i-1] = "FORK_GAUCHE";
                m_partChangesSujet1[yPos+PART_LENGTH_CHOICE-i-1] = "FORK_DROITE";
            }
            break;
        }
        case 7:
        {
            for(int i=0; i<DEPLACEMENT+1; i++)
            {
                m_partChangesSujet2[yPos+PART_LENGTH_CHOICE-i-1] = "FORK_DROITE";
                m_partChangesSujet1[yPos+PART_LENGTH_CHOICE-i-1] = "FORK_GAUCHE";
            }
            break;
        }
        }
    }
    else if(type == 1)
    {
        for(int i=0; i<DEPLACEMENT+1; i++)
        {
            m_partChangesSujet1[yPos-i-1] = "BODY_GAUCHE";
            m_partChangesSujet2[yPos-i-1] = "BODY_GAUCHE";
        }
    }
    else if(type == 2)
    {
        for(int i=0; i<DEPLACEMENT+1; i++)
        {
            m_partChangesSujet1[yPos-i-1] = "BODY_DROITE";
            m_partChangesSujet2[yPos-i-1] = "BODY_DROITE";
        }
    }
    else if(type == 0 && subtype == 1)
    {
        for(int i=0; i<DEPLACEMENT+1; i++)
        {
            m_partChangesSujet1[yPos-i-1] = "\0";
            m_partChangesSujet2[yPos-i-1] = "\0";
        }
    }
    else if(type == 0 && subtype == 0 && yPos >= PART_LENGTH_FORK && yPos <= PATH_LENGTH)
    {
        for(int i=0; i<DEPLACEMENT+1; i++)
        {
            m_partChangesSujet1[yPos-i-1] = "\0";
            m_partChangesSujet2[yPos-i-1] = "\0";
        }
    }
    else
    {
        QMessageBox::critical(this, "Error", "Unable to create partChanges");
        std::cout << "Erreur dans la creation du vecteur partChanges "<< type << " " << subtype << " " << yPos << std::endl;
    }
}

void MainWindow::generatePartGroten(CImg<unsigned char> &img, int subjectNumber, int partLength, int pos, int type, int subtype)
{
    float gauche = 2*PART_WIDTH/5.0;
    float milieu = PART_WIDTH/2.0;
    float droite = 3*PART_WIDTH/5.0;

    if(type == 0) //Ligne droite
    {
        //img.draw_line(milieu, 0+pos, 0, milieu, partLength+pos, 0, WHITE);
        drawLine(img,milieu, 0+pos, milieu, partLength+pos, WHITE, LINE_WIDTH);
    }
    else if(type == 1) //sinusoide gauche
    {
        for(int i=0+pos - 2*LINE_WIDTH/3; i<=partLength+pos+2*LINE_WIDTH/3; i++)
        {
            int x = milieu+(milieu-gauche)*(cos((i-pos)/(float)partLength*2.0*M_PI)-1)/2;
            int y = i;

            //img.draw_line(x, y, 0, x, y, 0, WHITE);
            img.draw_circle(x, y,LINE_WIDTH,WHITE);
        }
    }
    else if(type == 2) //sinusoide droite
    {
        for(int i=0+pos - 2*LINE_WIDTH/3; i<=partLength+pos+2*LINE_WIDTH/3; i++)
        {
            int x = milieu-(milieu-gauche)*(cos((i-pos)/(float)partLength*2.0*M_PI)-1)/2;
            int y = i;

            //img.draw_line(x, y, 0, x, y, 0, WHITE);
            img.draw_circle(x, y,LINE_WIDTH,WHITE);
        }
    }
    else if(type == 3 && FORK_TYPE == "DROIT")
    {
        if((subjectNumber == 1 && (subtype == 0 || subtype == 2 || subtype == 6)) //gauche gras
                || (subjectNumber == 2 && (subtype == 0 || subtype == 4 || subtype == 7)))
        {
            img.draw_circle(milieu, 0+pos, LINE_WIDTH_BOLD/2, LIGHTGREY);
            img.draw_circle(milieu, partLength+pos, (LINE_WIDTH_BOLD-1)/2, LIGHTGREY);


            drawLine(img,milieu - 2*LINE_WIDTH/3, 0 +pos, gauche, 0 + pos, LIGHTGREY, LINE_WIDTH_BOLD);
            drawLine(img, gauche, 0 + pos, gauche, partLength + pos, LIGHTGREY, LINE_WIDTH_BOLD);
            drawLine(img, gauche, partLength + pos, milieu - 2*LINE_WIDTH/3, partLength + pos, LIGHTGREY, LINE_WIDTH_BOLD);

            img.draw_circle(gauche, 0+pos, (LINE_WIDTH_BOLD-1)/2, LIGHTGREY);
            img.draw_circle(gauche, partLength+pos, LINE_WIDTH_BOLD/2, LIGHTGREY);

            drawLine(img, milieu, 0 + pos - LINE_WIDTH_BOLD, milieu, 0 + pos, WHITE, LINE_WIDTH);
            drawLine(img, milieu, partLength + pos + LINE_WIDTH_BOLD, milieu, partLength + pos, WHITE, LINE_WIDTH);

            drawLine(img, milieu, 0 +pos, gauche, 0 + pos, WHITE, LINE_WIDTH);
            drawLine(img, gauche, 0 + pos, gauche, partLength + pos, WHITE, LINE_WIDTH);
            drawLine(img, gauche, partLength + pos, milieu, partLength + pos, WHITE, LINE_WIDTH);

            drawLine(img, milieu, 0 +pos, droite, 0 + pos, WHITE, LINE_WIDTH);
            drawLine(img, droite, 0 + pos, droite, partLength + pos, WHITE, LINE_WIDTH);
            drawLine(img, droite, partLength + pos, milieu, partLength + pos, WHITE, LINE_WIDTH);

        }
        else if((subjectNumber == 1 && (subtype == 1 || subtype == 3 || subtype == 7)) //droite gras
                || (subjectNumber == 2 && (subtype == 1 || subtype == 5 || subtype == 6)))
        {
            img.draw_circle(milieu, 0+pos, LINE_WIDTH_BOLD/2, LIGHTGREY);
            img.draw_circle(milieu, partLength+pos, (LINE_WIDTH_BOLD-1)/2, LIGHTGREY);

            drawLine(img, milieu + 2*LINE_WIDTH/3, 0 +pos, droite, 0 + pos, LIGHTGREY, LINE_WIDTH_BOLD);
            drawLine(img, droite, 0 + pos, droite, partLength + pos, LIGHTGREY, LINE_WIDTH_BOLD);
            drawLine(img, droite, partLength + pos, milieu + 2*LINE_WIDTH/3, partLength + pos, LIGHTGREY, LINE_WIDTH_BOLD);

            img.draw_circle(droite, 0+pos, (LINE_WIDTH_BOLD-1)/2, LIGHTGREY);
            img.draw_circle(droite, partLength+pos, LINE_WIDTH_BOLD/2, LIGHTGREY);

            drawLine(img, milieu, 0 + pos - LINE_WIDTH_BOLD, milieu, 0 + pos, WHITE, LINE_WIDTH);
            drawLine(img, milieu, partLength + pos + LINE_WIDTH_BOLD, milieu, partLength + pos, WHITE, LINE_WIDTH);

            drawLine(img,milieu, 0 +pos, gauche, 0 + pos, WHITE, LINE_WIDTH);
            drawLine(img, gauche, 0 + pos, gauche, partLength + pos, WHITE, LINE_WIDTH);
            drawLine(img, gauche, partLength + pos, milieu, partLength + pos, WHITE, LINE_WIDTH);

            drawLine(img, milieu, 0 +pos, droite, 0 + pos, WHITE, LINE_WIDTH);
            drawLine(img, droite, 0 + pos, droite, partLength + pos, WHITE, LINE_WIDTH);
            drawLine(img, droite, partLength + pos, milieu, partLength + pos, WHITE, LINE_WIDTH);
        }
        else if((subjectNumber == 1 && (subtype == 4 || subtype == 5)) //pas de gras
                || (subjectNumber == 2 && (subtype == 2 || subtype == 3)))
        {
            drawLine(img,milieu, 0 +pos, gauche, 0 + pos, WHITE, LINE_WIDTH);
            drawLine(img, gauche, 0 + pos, gauche, partLength + pos, WHITE, LINE_WIDTH);
            drawLine(img, gauche, partLength + pos, milieu, partLength + pos, WHITE, LINE_WIDTH);

            drawLine(img, milieu, 0 +pos, droite, 0 + pos, WHITE, LINE_WIDTH);
            drawLine(img, droite, 0 + pos, droite, partLength + pos, WHITE, LINE_WIDTH);
            drawLine(img, droite, partLength + pos, milieu, partLength + pos, WHITE, LINE_WIDTH);
        }
    }
}

void MainWindow::generatePositionVectorGroten(int partLength, int pos, int type, int subtype)
{
    float gauche = 2*PART_WIDTH/5.0;
    float milieu = PART_WIDTH/2.0;
    float droite = 3*PART_WIDTH/5.0;

    if(type == 0) //ligne droite
    {
        for(int i=pos; i< pos+partLength; i++)
        {
            m_pathPos1[i] = milieu;
            m_pathPos2[i] = milieu;
        }
    }
    else if(type == 1) //sinusoide gauche
    {
        for(int i=pos; i< pos+partLength; i++)
        {
            m_pathPos1[i] = milieu + (milieu-gauche)*(cos((i-pos)/(float)partLength*2.0*M_PI)-1)/2.0;
            m_pathPos2[i] = milieu + (milieu-gauche)*(cos((i-pos)/(float)partLength*2.0*M_PI)-1)/2.0;
        }
    }
    else if(type == 2) //sinusoide droite
    {
        for(int i=pos; i< pos+partLength; i++)
        {
            m_pathPos1[i] = milieu - (milieu-gauche)*(cos((i-pos)/(float)partLength*2.0*M_PI)-1)/2.0;
            m_pathPos2[i] = milieu - (milieu-gauche)*(cos((i-pos)/(float)partLength*2.0*M_PI)-1)/2.0;
        }
    }
    else if(type == 3 && FORK_TYPE == "DROIT")
    {
        for(int i=pos; i<pos+partLength; i++)
        {
            switch(subtype)
            {
            case 0:
            {
                m_pathPos1[i] = gauche;
                m_pathPos2[i] = gauche;
                break;
            }
            case 1:
            {
                m_pathPos1[i] = droite;
                m_pathPos2[i] = droite;
                break;
            }
            case 2:
            {
                m_pathPos1[i] = gauche;
                m_pathPos2[i] = 10000;
                break;
            }
            case 3:
            {
                m_pathPos1[i] = droite;
                m_pathPos2[i] = 10000;
                break;
            }
            case 4:
            {
                m_pathPos1[i] = 10000;
                m_pathPos2[i] = gauche;
                break;
            }
            case 5:
            {
                m_pathPos1[i] = 10000;
                m_pathPos2[i] = droite;
                break;
            }
            case 6:
            {
                m_pathPos1[i] = gauche;
                m_pathPos2[i] = droite;
                break;
            }
            case 7:
            {
                m_pathPos1[i] = droite;
                m_pathPos2[i] = gauche;
                break;
            }
            }
        }
    }

}

bool MainWindow::generatePATH_obstacle(CImg<unsigned char> &img1, CImg<unsigned char> &img2)
{

    &img1.fill(BLACK[0], BLACK[1], BLACK[2]);
    &img2.fill(BLACK[0], BLACK[1], BLACK[2]);


    float node_pos = 0;
    float next_node_pos = 0;
    float node_time = 0;
    float next_node_time = 0;
    int connec_type = 0;
    int node_pix = 0;
    int next_node_pix = 0;

    QString scenario_file_name = "../scenarios/obstacle/SCENARIO_OBST_" + QString::fromStdString(server->infos_UI[2]);
    QString obstacle_file_name = "../scenarios/obstacle/OBSTACLES_" + QString::fromStdString(server->infos_UI[2]) +".txt";

    QFile scenarioFile(scenario_file_name);
    QFile obstacleFile(obstacle_file_name);

    if(!scenarioFile.open(QFile::ReadOnly))
    {
        QMessageBox::critical(this, "Error", "Unable to open scenario file");
        std::cout << "Error: Unable to open scenario file" << std::endl;
        return false;
    }
    if(!obstacleFile.open(QFile::ReadOnly))
    {
        QMessageBox::critical(this, "Error", "Unable to open obstacle file");
        std::cout << "Error: Unable to open obstacle file" << std::endl;
        return false;
    }

    QTextStream fileStream(&scenarioFile);
    QTextStream obstacleStream(&obstacleFile);

    int yPos = PATH_LENGTH;
    int limite = PART_LENGTH_BODY*3;
    //int partWithObstacle = 0;

    m_partChangesSujet1.clear();
    m_partChangesSujet1.assign(PATH_LENGTH,"\0");
    m_partChangesSujet2.clear();
    m_partChangesSujet2.assign(PATH_LENGTH,"\0");

    //The loop is reversed to start at the bottom of the path's surface ( = at the start)
    //--------------------------------
    while(yPos > 0)
    {
        if( yPos >= limite)
        {
            QString line = fileStream.readLine();
            QStringList splitList = line.split("\t");

            node_pos = MILIEU + splitList[0].toFloat()*WINDOW_WIDTH/100;
            next_node_pos = MILIEU + splitList[1].toFloat()*WINDOW_WIDTH/100;
            node_time = splitList[2].toFloat();
            next_node_time = splitList[3].toFloat();
            connec_type = splitList[4].toInt();
            node_pix = node_time*VITESSE;
            next_node_pix = next_node_time*VITESSE;
        }
        else // Straight line at the end
        {
            node_pos= next_node_pos;
            node_pix=next_node_pix;
            next_node_pix=PATH_LENGTH;
            connec_type=0;
        }
//            editTransition_obstacle(yPos, type, subtype);

        generatePositionVector_obstacle(img1, img2, node_pos, next_node_pos, PATH_LENGTH - node_pix, PATH_LENGTH - next_node_pix, connec_type);

        yPos -= (next_node_pix-node_pix);
    }
    if(atof(server->infos_UI[7].c_str()) == 1){
        for(int i = 0; i < PATH_LENGTH; i++){
            img1.draw_circle((int)m_pathPos1[i], i,LINE_WIDTH,WHITE);
        }
    }
    if(atof(server->infos_UI[8].c_str()) == 1){
        for(int i = 0; i < PATH_LENGTH; i++){
            img2.draw_circle((int)m_pathPos2[i], i,LINE_WIDTH,WHITE);
        }
    }

    while(1){
        QString line = obstacleStream.readLine();
        QStringList splitList = line.split("\t");

        int x_obst = splitList[0].toInt();
        int y_obst = splitList[1].toInt();
        int radius_obst = splitList[2].toInt();
        std::vector<int> obstacle ({x_obst, y_obst, radius_obst});
        m_obstacles.push_back(obstacle);

        if (y_obst >= PATH_LENGTH){
            break;
        }

        if(atof(server->infos_UI[9].c_str()) == 1){
            img1.draw_circle(x_obst, PATH_LENGTH - y_obst, radius_obst, RED);
        }
        if(atof(server->infos_UI[10].c_str()) == 1){
            img2.draw_circle(x_obst, PATH_LENGTH - y_obst, radius_obst, RED);
        }
        img_obstacles.draw_circle(x_obst, PATH_LENGTH - y_obst, radius_obst+CURSOR_WIDTH, RED);
    }
    img_obstacles.save_jpeg("./IMG_OBST.jpg");
    CImg<unsigned char> image("IMG_OBST.jpg");
    img_obstacles = image;
//    for(int i = 0; i < img_obstacles.width(); i++){
//        for(int j =0; j< img_obstacles.height(); j++){
//            unsigned char value_red = img_obstacles(i, j, 0, 0);
//            unsigned char value_green = img_obstacles(i, j, 0, 1);
//            unsigned char value_blue = img_obstacles(i, j, 0, 2);
//            if(value_red>0){
//                std::cerr << i << '\t' << j << '\t' << (int)value_red << '\t' << (int)value_green << '\t' << (int)value_blue << '\n';
//            }
//        }
//    }

    scenarioFile.close();
    obstacleFile.close();
    return true;
}


void MainWindow::generatePositionVector_obstacle(CImg<unsigned char> &img1, CImg<unsigned char> &img2, float node_pos, float next_node_pos, int node_pix, int next_node_pix, int connec_type)
{
    if(connec_type == 0){
        for (int i=node_pix; i>=next_node_pix; i--){
            m_pathPos1[i] = (node_pos + (float)(next_node_pos - node_pos)/(float)(next_node_pix-node_pix)*(float)(i - node_pix));
            m_pathPos2[i] = m_pathPos1[i];
        }
    }
    if(connec_type == 1){
        for (int i=node_pix; i>=next_node_pix; i--){
            m_pathPos1[i] = node_pos + (float)(next_node_pos-node_pos)*(1-cos(3.14159*(float)(i - node_pix)/(float)(next_node_pix-node_pix)))/2;
            m_pathPos2[i] = m_pathPos1[i];
        }
    }
    if(connec_type == 2){
        float middle = node_pix - (node_pix - next_node_pix)/2;
        for (int i=node_pix; i>=next_node_pix; i--){
            if(i > middle){
                m_pathPos1[i] = node_pos;
            }
            else{
                m_pathPos1[i] = next_node_pos;
            }
            m_pathPos2[i] = m_pathPos1[i];
        }
        if(atof(server->infos_UI[7].c_str()) == 1){
            drawLine(img1, node_pos, middle, next_node_pos, middle, WHITE, LINE_WIDTH);
        }
        if(atof(server->infos_UI[8].c_str()) == 1){
            drawLine(img2, node_pos, middle, next_node_pos, middle, WHITE, LINE_WIDTH);
        }
    }
}



void MainWindow::drawLine(cimg_library::CImg<unsigned char>& img, int x0, int y0, int x1, int y1, const unsigned char* color, int size, float opacity)
{
    if(size == 1)
    {
        img.draw_line(x0,y0,0,x1,y1,0,color,opacity);
    }
    else
    {
        CImg<unsigned char> tmp(img.width(), img.height());
        tmp.fill(0);

        tmp.draw_line(x0,y0,0,x1,y1,0,WHITE);

        cimg_forXY(tmp,x,y)
        {
            if(tmp(x,y) == 255)
            {
                img.draw_circle(x,y,size,color,opacity);
            }
        }
    }
}

bool MainWindow::terminateAndClear(int record)
{
    m_checkIsAliveTimer.stop();
    m_refreshUITimer.stop();
    //m_getPositionTimer.stop();
    threadReceivePosAlive = 0;
    scenarioCharged = 0;
    usleep(10000);
    m_imagePath1->clear();
    m_imagePath2->clear();

    //std::unique_lock<std::mutex> lock(mtx);
    //lock.unlock();
    //cv.notify_one();
//    m_paddleUser1.sendMessage("0\0",UDP_PORT3);

    //ui->pushButton_start->setText("START");

    if (record == 0)
    {
        CImg<unsigned char> image1("isir.jpg");
        CImg<unsigned char> image2("isir.jpg");
        m_disp1->display(image1);
        m_disp2->display(image2);
        m_paddleUser1.sendMessage("STOP\0",UDP_PORT3);
        if(display_score){
            m_score_display1->close();
            m_score_display2->close();
        }
    }

    //m_paddleUser1.sendMessage("STOP\0");

    else if(record == 1)
    {
        CImg<unsigned char> image1(WINDOW_WIDTH,WINDOW_LENGTH,1,3);
        CImg<unsigned char> image2(WINDOW_WIDTH,WINDOW_LENGTH,1,3);
        image1.fill(BLACK[0], BLACK[1], BLACK[2]);
        image2.fill(BLACK[0], BLACK[1], BLACK[2]);
        m_disp1->display(image1);
        m_disp2->display(image2);
        usleep(10000);
        // send mode to robot
        m_paddleUser1.sendMessage("0",UDP_PORT3);
        usleep(10000);
        m_paddleUser1.sendMessage("STOPANDSAVE\0",UDP_PORT3);

        // get logs
        QString date = QDateTime::currentDateTime().toString("dd-MM-hh-mm");

        QString scenarioNumber = QString::fromStdString(server->infos_UI[2]);
        QString subjectName1 = QString::fromStdString(server->infos_UI[0]);
        QString subjectName2 = QString::fromStdString(server->infos_UI[1]);
        QString trialNumber = QString::fromStdString(server->infos_UI[3]);
        QString feedBack = QString::fromStdString(server->infos_UI[4]);
        if(feedBack == "0"){
            feedBack = "Alone";}
        else if(feedBack == "1"){
            feedBack = "HFOP";}
        else if (feedBack == "2"){
            feedBack = "HRP";}
        else if(feedBack == "3"){
            feedBack = "HFO";}
        else if(feedBack == "4"){
            feedBack = "KRP";}
        QString logfile_name = "../results/RESULTS_scenario_" + scenarioNumber + "_trial_" + trialNumber + "_" + feedBack + "_" + date + ".txt";
        QFile logFile(logfile_name);

        if(!logFile.open(QFile::WriteOnly))
        {
            QMessageBox::critical(this, "Error", "Unable to create log file");
            return false;
        }

        QTextStream logStream(&logFile);

        logStream << "PARAMETERS\n";
        logStream << "\n";
        logStream << "SUBJECT NAME1 : " << subjectName1 << "\n";
        logStream << "SUBJECT NAME2 : " << subjectName2 << "\n";
        logStream << "SCENARIO : " << scenarioNumber << "\n";
        logStream << "TRIAL : " << trialNumber << "\n";
        logStream << "\n";
        logStream << "PATH_DURATION : " << PATH_DURATION << "\n";
        logStream << "VITESSE : " << VITESSE << "\n";
        logStream << "Y_POS_CURSOR : " << Y_POS_CURSOR << "\n";
        logStream << "PART_DURATION_BODY : " << PART_DURATION_BODY << "\n";
        logStream << "PART_DURATION_CHOICE : " << PART_DURATION_CHOICE << "\n";
        logStream << "PART_DURATION_FORK : " << PART_DURATION_FORK << "\n";
        logStream << "PART_DURATION_REGRP : " << PART_DURATION_REGRP << "\n";
        logStream << "PART_DURATION_START : " << PART_DURATION_START << "\n";
        logStream << "POSITION_OFFSET : " << POSITION_OFFSET << "\n";
        logStream << "SENSITIVITY : " << SENSITIVITY << "\n";
        logStream << "WINDOW_WIDTH : " << WINDOW_WIDTH << "\n";
        logStream << "WINDOW_LENGTH : " << WINDOW_LENGTH << "\n";
        logStream << "\n";
        logStream << "SCORE 1 : " << score1 << "\n";
        logStream << "SCORE 2 : " << score2 << "\n";
        logStream << "\n";
        logStream << "RESULTS\n";
        logStream << "\n";
        logStream << "ROBOT TIME" << "\t" << "PATH POSITION 1" << "\t" << "PATH POSITION 2" << "\t" << "POSITION SUBJECT 1" << "\t" << "POSITION SUBJECT 2" << "\t" << "FORCE SUBJECT 1" << "\t" << "FORCE SUBJECT 2" << "\t" << "CONSIGNE 1" << "\t" << "CONSIGNE 2" << "\t" << "POSITION ROBOT 1"  << "\t" << "POSITION ROBOT 2"<< "\t" << "PART CHANGES 1" << "\t" << "PART CHANGES 2 " << "\n";
        logStream << "\n";

        QString paddleDataFileName = "../data/data.txt";

        sleep(10);

        system("scp ubuntu@192.168.0.2:/home/ubuntu/TEST/controller/data/data.txt /home/roche/phri/lucas/data/");

        /*if(!m_paddleUser1.getDataFile(paddleDataFileName.toStdString()))
        {
            QMessageBox::critical(this, "Error", "Unable to get paddle data file");
            return false;
        }*/

        QFile paddleDataFile(paddleDataFileName);
        if(!paddleDataFile.open(QFile::ReadOnly))
        {
            QMessageBox::critical(this, "Error", "Unable to open paddle data file");
            return false;
        }

        QString line;
        while(!paddleDataFile.atEnd())
        {
            line = paddleDataFile.readLine();
        }
        std::cout << line.toStdString() << std::endl;
        QStringList splitList = line.split("\t");

        float finalTime;
        finalTime = splitList[0].toFloat();

        float tmp1 = Y_POS_CURSOR;
        float tmp2 = VITESSE;
        float timeOffset = tmp1/tmp2;
        float facteurDilatation = (PATH_DURATION+float(WINDOW_LENGTH)/VITESSE)/finalTime;

        float timePaddle;
        float pos1;
        float pos2;
        float consigne1;
        float consigne2;
        float force1;
        float force2;
        float timePaddleModified;
        float obstaclePos = -1;
        float posVirtuelle1;
        float posVirtuelle2;
        float pathPos1;
        float pathPos2;
        char partChange1[15];
        char partChange2[15];

        paddleDataFile.reset();
        while(!paddleDataFile.atEnd())
        {
            line = paddleDataFile.readLine();
            splitList = line.split("\t");
            timePaddle = splitList[0].toFloat();
            pos1 = splitList[1].toFloat();
            consigne1 = splitList[2].toFloat();
            force1 = splitList[3].toFloat();
            pos2 = splitList[4].toFloat();
            consigne2 = splitList[5].toFloat();
            force2 = splitList[6].toFloat();
            timePaddleModified = timePaddle*facteurDilatation;
            posVirtuelle1 = splitList[7].toFloat();
            posVirtuelle2 = splitList[8].toFloat();

            if((timePaddleModified <= timeOffset or int(timePaddleModified-timeOffset) >= PATH_DURATION))
            {
                pathPos1 = -1;
                pathPos2 = -1;
                strcpy(partChange1, "\0");
                strcpy(partChange2, "\0");
            }
            else
            {
                pathPos1 = m_pathPos1[PATH_LENGTH - int((timePaddleModified-timeOffset)*VITESSE)];
                pathPos2 = m_pathPos2[PATH_LENGTH - int((timePaddleModified-timeOffset)*VITESSE)];
                int n = PATH_LENGTH - int((timePaddleModified-timeOffset)*VITESSE);
                if(n>=m_partChangesSujet1.size()){n=m_partChangesSujet1.size() - 1;}
                strcpy(partChange1, m_partChangesSujet1[n].c_str());
                strcpy(partChange2, m_partChangesSujet2[n].c_str());
            }

            logStream << timePaddle << "\t" << pathPos1 << "\t" << pathPos2 << "\t" << pos1 << "\t" << pos2 << "\t" << force1 << "\t" << force2 << "\t" << consigne1 << "\t" << consigne2 << "\t"<< posVirtuelle1 << "\t" << posVirtuelle2  << "\t" << partChange1 << "\t" << partChange2 << "\n";
        }

        paddleDataFile.close();
        logFile.close();

        if(record_film){
            QString cmd = "tar -czf ../results/"+date+"_recording.tar ../recording/";
            system(cmd.toStdString().c_str());
        }

    }
    if(display_score){
        m_score_display1->close();
        m_score_display2->close();
    }
    CImg<unsigned char> image1bis("isir.jpg");
    CImg<unsigned char> image2bis("isir.jpg");
    m_disp1->display(image1bis);
    m_disp2->display(image2bis);
    if(record_film){
        system("rm ../recording/*");
    }

    return true;

}


void MainWindow::mode_change(int index)
{
    std::string message;
    int PREV_MODE = MODE;
    MODE = index;
//    if((MODE == 1 || MODE == 2 || MODE == 4) && (PREV_MODE == 0 || PREV_MODE == 3)){
//        this->centering(1);
//    }
    this->centering(1);
    message = std::to_string(index);
    m_paddleUser1.sendMessage(message,UDP_PORT3);

    std::cout << "MODE = " << index << "\n";
}

void MainWindow::centering(int image)
{
    CImg<unsigned char> image1("centering.jpg");
    CImg<unsigned char> image2("centering.jpg");
    CImg<unsigned char> image1bis("isir.jpg");
    CImg<unsigned char> image2bis("isir.jpg");
    m_disp1->display(image1);
    m_disp2->display(image2);
    m_paddleUser1.sendMessage("CENTER",UDP_PORT3);
    sleep(1);
    if( image == 0){
        image1.fill(BLACK[0], BLACK[1], BLACK[2]);
        image2.fill(BLACK[0], BLACK[1], BLACK[2]);
        m_disp1->display(image1);
        m_disp2->display(image2);
    }
    else if (image ==1){

        m_disp1->display(image1bis);
        m_disp2->display(image2bis);
    }

}


void send_part_changes()
{
    int sock_sdprt1, sock_sdprt2, n, optval =1;
    struct sockaddr_in sock_struct_sdprt1;
    struct sockaddr_in sock_struct_sdprt2;

    char buffer[256];

    // Creating socket structures
    bzero((char *) &sock_struct_sdprt1, sizeof(sock_struct_sdprt1));
    sock_struct_sdprt1.sin_family = AF_INET;
    inet_aton(TCP_IP, &sock_struct_sdprt1.sin_addr);
    sock_struct_sdprt1.sin_port = htons(TCP_PORT3);

    bzero((char *) &sock_struct_sdprt2, sizeof(sock_struct_sdprt2));
    sock_struct_sdprt2.sin_family = AF_INET;
    inet_aton(TCP_IP, &sock_struct_sdprt2.sin_addr);
    sock_struct_sdprt2.sin_port = htons(TCP_PORT4);

    while(threadSendPartAlive)
    {
        std::cerr << "Salut" << "\n";
        // lock until signal given in MainWindow::on_refresh
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock);
        if(!threadSendPartAlive)
        {
            break;
        }

        std::cerr << "Mess 1 :" << ptChgMessage1 << "\n";
        std::cerr << "Mess 2 :" << ptChgMessage2 << "\n";

        //Create sockets
        sock_sdprt1 = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_sdprt1 < 0)
            std::cerr << "ERROR opening socket"<<std::endl;
        sock_sdprt2 = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_sdprt2 < 0)
            std::cerr << "ERROR opening socket"<<std::endl;
        setsockopt(sock_sdprt1, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        setsockopt(sock_sdprt2, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        setsockopt(sock_sdprt1, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
        setsockopt(sock_sdprt2, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

        //Connect to sockets
        if (connect(sock_sdprt1,(struct sockaddr *) &sock_struct_sdprt1,sizeof(sock_struct_sdprt1)) < 0)
            std::cerr << "ERROR connecting to socket"<<std::endl;
        if (connect(sock_sdprt2,(struct sockaddr *) &sock_struct_sdprt2,sizeof(sock_struct_sdprt2)) < 0)
            std::cerr << "ERROR connecting to socket"<<std::endl;

        //Send message through socket 1
        bzero(buffer,256);
        sprintf(buffer, "%s", ptChgMessage1);
        n = send(sock_sdprt1,buffer,strlen(buffer),0);
        if (n < 0)
             std::cerr << "ERROR writing to socket"<<std::endl;
        bzero(buffer,256);
        n = recv(sock_sdprt1,buffer,255, 0);
        if (n < 0)
             std::cerr << "ERROR reading from socket"<<std::endl;

        //Send message through socket 2
        bzero(buffer,256);
        sprintf(buffer, "%s", ptChgMessage2);
        n = send(sock_sdprt2,buffer,strlen(buffer),0);
        if (n < 0)
             std::cerr << "ERROR writing to socket"<<std::endl;
        bzero(buffer,256);
        n = recv(sock_sdprt2,buffer,255, 0);
        if (n < 0)
             std::cerr << "ERROR reading from socket"<<std::endl;

        close(sock_sdprt1);
        close(sock_sdprt2);
    }

}

void MainWindow::close_GUI()
{
    this->~MainWindow();
}

