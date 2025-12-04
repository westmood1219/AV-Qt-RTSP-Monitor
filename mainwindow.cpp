#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QMessageBox>
#include <qpushbutton.h>
#include "ui_mainwindow.h"

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //解耦合后

    //创建一个中心Widget作为布局容器
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    //1 创建2*2网格布局
    QGridLayout *layout = new QGridLayout(centralWidget);

    //示例RTSP流
    QString streamUrl = "rtsp://10.163.18.72:8090/h264_pcm.sdp";

    //2 实例化4个流(暂时用同一个代替)并加入网格
    for(int i=0;i<2;++i){
        for(int j=0;j<2;j++){
            RTSPPlayer *player = new RTSPPlayer(centralWidget);
            connect(player, &RTSPPlayer::sig_doubleClick, this, &MainWindow::onPlayerDoubleClicked);
            // 存到列表里，待会要用来循环
            players.append(player);

            layout->addWidget(player,i,j);

            //play
            player->play(streamUrl.arg(i).arg(j));
        }
    }
    //调整布局大小策略
    centralWidget->setLayout(layout);
}

void MainWindow::onPlayerDoubleClicked(QWidget *clickedWidget)
{
    if (isMaximizedState) {
        // 如果已经是全屏状态，那就——【还原】

        // 1. 把所有人都显示出来
        for(auto player : players) {
            player->setVisible(true);
        }

        // 2. 状态改回去
        isMaximizedState = false;

    } else {
        // 如果是普通状态，那就——【全屏】

        // 1. 遍历所有播放器
        for(auto player : players) {
            if (player == clickedWidget) {
                // 如果是刚才双击的那个，就显示
                player->setVisible(true);
            } else {
                // 其他的统统藏起来！
                player->setVisible(false);
            }
        }

        // 2. 状态改为全屏
        isMaximizedState = true;
    }
}

MainWindow::~MainWindow()
{
    for(auto player:players){
        player->stop();
    }

    delete ui;
}
