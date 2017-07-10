#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <vector>
#include <QThread>
#include <QWidget>
#include <QLabel>

#include "paddle.h"
#include "server.h"

// temporary class to bypass compiler errors
namespace cimg_library {
template<class T>
struct CImg;
struct CImgDisplay;
}

namespace Ui {

class MainWindow;
}


class MainWindow : public QMainWindow
{
    Q_OBJECT
    QThread* server_thread;

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    Paddle m_paddleUser1;
    Paddle m_paddleUser2;

//    Server server;

    QTimer m_refreshUITimer;
    QTimer m_checkIsAliveTimer;
    QTimer m_getPositionTimer;

    QLabel* m_score_display1;
    QLabel* m_score_display2;
    float score1;
    float score2;

    cimg_library::CImg<unsigned char>* m_imagePath1;
    cimg_library::CImg<unsigned char>* m_imagePath2;
    cimg_library::CImg<unsigned char>* m_obstacle_loc;
    cimg_library::CImgDisplay* m_disp1;
    cimg_library::CImgDisplay* m_disp2;

    float m_pos1;
    float m_pos2;
    float m_posVirt1;
    float m_posVirt2;
    float m_force1;
    float m_force2;

    std::vector<float> m_pathPos1;
    std::vector<float> m_pathPos2;
    std::vector<std::string> m_partChangesSujet1;
    std::vector<std::string> m_partChangesSujet2;
    std::vector<std::vector<int>> m_obstacles;

    float m_yPath;
    float m_xPosCursor1;
    float m_xOldPosCursor1;
    float m_xPosCursor2;
    float m_xOldPosCursor2;
    int m_frameNumber;

    bool generatePATH(cimg_library::CImg<unsigned char> &img1, cimg_library::CImg<unsigned char> &img2);

    bool generatePATH_obstacle(cimg_library::CImg<unsigned char> &img1, cimg_library::CImg<unsigned char> &img2);

    void editTransition(int yPos, int type, int subtype);

    void generatePartGroten(cimg_library::CImg<unsigned char> &img, int subjectNumber, int partLength, int pos, int type, int subtype);

    void generatePositionVectorGroten(int partLength, int pos, int type, int subtype);

    void generatePositionVector_obstacle(cimg_library::CImg<unsigned char> &img1, cimg_library::CImg<unsigned char> &img2, float node_pos, float next_node_pos, int node_pix, int next_node_pix, int connec_type);
    //void generateCURSOR(int x, int y, unsigned char color[3], cimg_library::CImg<unsigned char>* img);

    void drawLine(cimg_library::CImg<unsigned char>& img, int x0, int y0, int x1, int y1, const unsigned char* color, int size = 1, float opacity = 1 );

    bool terminateAndClear(int record);

    void centering(int image);



public slots:
    void on_start();
    void on_refresh();
    void on_alive();
    void getPaddlePositions();
    void mode_change(int);
    void close_GUI();
    void on_load();
//    void send_part_changes();
//    void Server();

};

#endif // MAINWINDOW_H
