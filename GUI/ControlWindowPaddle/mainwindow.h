#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

public slots:
    void on_start();

private slots:
    void on_comboBox_feedBack_currentIndexChanged(int index);
    void on_pushButton_load_clicked();
    void on_comboBox_Expe_currentIndexChanged(int index);
};

#endif // MAINWINDOW_H
