/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.5.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralWidget;
    QWidget *widget;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QLabel *label_Experience;
    QComboBox *comboBox_Expe;
    QCheckBox *checkBox_Score;
    QHBoxLayout *horizontalLayout_7;
    QLabel *label_nameSubject1;
    QLineEdit *lineEdit_nameSubject1;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label_nameSubject2;
    QLineEdit *lineEdit_nameSubject2;
    QHBoxLayout *horizontalLayout_3;
    QLabel *label_scenarioNumber;
    QLineEdit *lineEdit_scenarioNumber;
    QHBoxLayout *horizontalLayout_4;
    QLabel *label_feedBack;
    QComboBox *comboBox_feedBack;
    QHBoxLayout *horizontalLayout_5;
    QLabel *label_trialNumber;
    QLineEdit *lineEdit_trialNumber;
    QHBoxLayout *horizontalLayout_6;
    QPushButton *pushButton_load;
    QLabel *label_duration;
    QLineEdit *lineEdit_duration;
    QPushButton *pushButton_start;
    QHBoxLayout *horizontalLayout_8;
    QLabel *label_Path;
    QCheckBox *checkBox_Path1;
    QCheckBox *checkBox_Path2;
    QLabel *label_Obstacle;
    QCheckBox *checkBox_Obst1;
    QCheckBox *checkBox_Obst2;
    QToolBar *mainToolBar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QStringLiteral("MainWindow"));
        MainWindow->resize(319, 418);
        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        widget = new QWidget(centralWidget);
        widget->setObjectName(QStringLiteral("widget"));
        widget->setGeometry(QRect(10, 10, 291, 371));
        verticalLayout = new QVBoxLayout(widget);
        verticalLayout->setSpacing(6);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(6);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        label_Experience = new QLabel(widget);
        label_Experience->setObjectName(QStringLiteral("label_Experience"));

        horizontalLayout->addWidget(label_Experience);

        comboBox_Expe = new QComboBox(widget);
        comboBox_Expe->setObjectName(QStringLiteral("comboBox_Expe"));

        horizontalLayout->addWidget(comboBox_Expe);

        checkBox_Score = new QCheckBox(widget);
        checkBox_Score->setObjectName(QStringLiteral("checkBox_Score"));

        horizontalLayout->addWidget(checkBox_Score);


        verticalLayout->addLayout(horizontalLayout);

        horizontalLayout_7 = new QHBoxLayout();
        horizontalLayout_7->setSpacing(6);
        horizontalLayout_7->setObjectName(QStringLiteral("horizontalLayout_7"));
        label_nameSubject1 = new QLabel(widget);
        label_nameSubject1->setObjectName(QStringLiteral("label_nameSubject1"));
        label_nameSubject1->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout_7->addWidget(label_nameSubject1);

        lineEdit_nameSubject1 = new QLineEdit(widget);
        lineEdit_nameSubject1->setObjectName(QStringLiteral("lineEdit_nameSubject1"));

        horizontalLayout_7->addWidget(lineEdit_nameSubject1);


        verticalLayout->addLayout(horizontalLayout_7);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setSpacing(6);
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        label_nameSubject2 = new QLabel(widget);
        label_nameSubject2->setObjectName(QStringLiteral("label_nameSubject2"));
        label_nameSubject2->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout_2->addWidget(label_nameSubject2);

        lineEdit_nameSubject2 = new QLineEdit(widget);
        lineEdit_nameSubject2->setObjectName(QStringLiteral("lineEdit_nameSubject2"));

        horizontalLayout_2->addWidget(lineEdit_nameSubject2);


        verticalLayout->addLayout(horizontalLayout_2);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setSpacing(6);
        horizontalLayout_3->setObjectName(QStringLiteral("horizontalLayout_3"));
        label_scenarioNumber = new QLabel(widget);
        label_scenarioNumber->setObjectName(QStringLiteral("label_scenarioNumber"));
        label_scenarioNumber->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout_3->addWidget(label_scenarioNumber);

        lineEdit_scenarioNumber = new QLineEdit(widget);
        lineEdit_scenarioNumber->setObjectName(QStringLiteral("lineEdit_scenarioNumber"));

        horizontalLayout_3->addWidget(lineEdit_scenarioNumber);


        verticalLayout->addLayout(horizontalLayout_3);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setSpacing(6);
        horizontalLayout_4->setObjectName(QStringLiteral("horizontalLayout_4"));
        label_feedBack = new QLabel(widget);
        label_feedBack->setObjectName(QStringLiteral("label_feedBack"));
        label_feedBack->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout_4->addWidget(label_feedBack);

        comboBox_feedBack = new QComboBox(widget);
        comboBox_feedBack->setObjectName(QStringLiteral("comboBox_feedBack"));

        horizontalLayout_4->addWidget(comboBox_feedBack);


        verticalLayout->addLayout(horizontalLayout_4);

        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setSpacing(6);
        horizontalLayout_5->setObjectName(QStringLiteral("horizontalLayout_5"));
        label_trialNumber = new QLabel(widget);
        label_trialNumber->setObjectName(QStringLiteral("label_trialNumber"));
        label_trialNumber->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout_5->addWidget(label_trialNumber);

        lineEdit_trialNumber = new QLineEdit(widget);
        lineEdit_trialNumber->setObjectName(QStringLiteral("lineEdit_trialNumber"));

        horizontalLayout_5->addWidget(lineEdit_trialNumber);


        verticalLayout->addLayout(horizontalLayout_5);

        horizontalLayout_6 = new QHBoxLayout();
        horizontalLayout_6->setSpacing(6);
        horizontalLayout_6->setObjectName(QStringLiteral("horizontalLayout_6"));
        pushButton_load = new QPushButton(widget);
        pushButton_load->setObjectName(QStringLiteral("pushButton_load"));

        horizontalLayout_6->addWidget(pushButton_load);

        label_duration = new QLabel(widget);
        label_duration->setObjectName(QStringLiteral("label_duration"));
        label_duration->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout_6->addWidget(label_duration);

        lineEdit_duration = new QLineEdit(widget);
        lineEdit_duration->setObjectName(QStringLiteral("lineEdit_duration"));

        horizontalLayout_6->addWidget(lineEdit_duration);


        verticalLayout->addLayout(horizontalLayout_6);

        pushButton_start = new QPushButton(widget);
        pushButton_start->setObjectName(QStringLiteral("pushButton_start"));
        pushButton_start->setEnabled(false);

        verticalLayout->addWidget(pushButton_start);

        horizontalLayout_8 = new QHBoxLayout();
        horizontalLayout_8->setSpacing(6);
        horizontalLayout_8->setObjectName(QStringLiteral("horizontalLayout_8"));
        label_Path = new QLabel(widget);
        label_Path->setObjectName(QStringLiteral("label_Path"));
        label_Path->setEnabled(false);

        horizontalLayout_8->addWidget(label_Path);

        checkBox_Path1 = new QCheckBox(widget);
        checkBox_Path1->setObjectName(QStringLiteral("checkBox_Path1"));
        checkBox_Path1->setEnabled(false);
        checkBox_Path1->setChecked(true);

        horizontalLayout_8->addWidget(checkBox_Path1);

        checkBox_Path2 = new QCheckBox(widget);
        checkBox_Path2->setObjectName(QStringLiteral("checkBox_Path2"));
        checkBox_Path2->setEnabled(false);
        checkBox_Path2->setChecked(true);

        horizontalLayout_8->addWidget(checkBox_Path2);

        label_Obstacle = new QLabel(widget);
        label_Obstacle->setObjectName(QStringLiteral("label_Obstacle"));
        label_Obstacle->setEnabled(false);

        horizontalLayout_8->addWidget(label_Obstacle);

        checkBox_Obst1 = new QCheckBox(widget);
        checkBox_Obst1->setObjectName(QStringLiteral("checkBox_Obst1"));
        checkBox_Obst1->setEnabled(false);
        checkBox_Obst1->setChecked(true);

        horizontalLayout_8->addWidget(checkBox_Obst1);

        checkBox_Obst2 = new QCheckBox(widget);
        checkBox_Obst2->setObjectName(QStringLiteral("checkBox_Obst2"));
        checkBox_Obst2->setEnabled(false);
        checkBox_Obst2->setChecked(true);

        horizontalLayout_8->addWidget(checkBox_Obst2);


        verticalLayout->addLayout(horizontalLayout_8);

        MainWindow->setCentralWidget(centralWidget);
        mainToolBar = new QToolBar(MainWindow);
        mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
        MainWindow->addToolBar(Qt::TopToolBarArea, mainToolBar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "MainWindow", 0));
        label_Experience->setText(QApplication::translate("MainWindow", "Experience", 0));
        comboBox_Expe->clear();
        comboBox_Expe->insertItems(0, QStringList()
         << QApplication::translate("MainWindow", "Symetrique", 0)
         << QApplication::translate("MainWindow", "ASymetrique", 0)
        );
        checkBox_Score->setText(QApplication::translate("MainWindow", "Score", 0));
        label_nameSubject1->setText(QApplication::translate("MainWindow", "Nom sujet 1", 0));
        lineEdit_nameSubject1->setText(QApplication::translate("MainWindow", "Test 1", 0));
        label_nameSubject2->setText(QApplication::translate("MainWindow", "Nom sujet 2", 0));
        lineEdit_nameSubject2->setText(QApplication::translate("MainWindow", "Test 2", 0));
        label_scenarioNumber->setText(QApplication::translate("MainWindow", "Num\303\251ro sc\303\251nario", 0));
        lineEdit_scenarioNumber->setText(QApplication::translate("MainWindow", "101", 0));
        label_feedBack->setText(QApplication::translate("MainWindow", "Feedback", 0));
        comboBox_feedBack->clear();
        comboBox_feedBack->insertItems(0, QStringList()
         << QApplication::translate("MainWindow", "Alone", 0)
         << QApplication::translate("MainWindow", "HFOP", 0)
         << QApplication::translate("MainWindow", "HRP", 0)
         << QApplication::translate("MainWindow", "HFO", 0)
         << QApplication::translate("MainWindow", "KRP", 0)
        );
        label_trialNumber->setText(QApplication::translate("MainWindow", "Num\303\251ro essai", 0));
        lineEdit_trialNumber->setText(QApplication::translate("MainWindow", "1", 0));
        pushButton_load->setText(QApplication::translate("MainWindow", "LOAD", 0));
        label_duration->setText(QApplication::translate("MainWindow", "Duree", 0));
        lineEdit_duration->setText(QApplication::translate("MainWindow", "110", 0));
        pushButton_start->setText(QApplication::translate("MainWindow", "START/STOP", 0));
        label_Path->setText(QApplication::translate("MainWindow", "Path", 0));
        checkBox_Path1->setText(QApplication::translate("MainWindow", "1", 0));
        checkBox_Path2->setText(QApplication::translate("MainWindow", "2", 0));
        label_Obstacle->setText(QApplication::translate("MainWindow", "Obstacles", 0));
        checkBox_Obst1->setText(QApplication::translate("MainWindow", "1", 0));
        checkBox_Obst2->setText(QApplication::translate("MainWindow", "2", 0));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
