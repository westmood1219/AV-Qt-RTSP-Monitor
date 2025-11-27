#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QFileDialog>   // ← 漏了这个
#include <QMessageBox>   // 可选，调试方便
#include <qpushbutton.h>

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

    /*打槽流程:
     1.mainwindow里引入封装的类
     2.声明私有的槽private slots:
     3.私有)实例化业务类
     4.mainwindow.cpp里connect连接信号与槽
tips:槽函数名字叫 on_btnOpen_clicked,在 Qt 中，这是一个非常特殊的命名格式。 如果你的 ui 文件里真的有一个按钮叫 btnOpen，那么 ui->setupUi(this) 会调用一个叫 QMetaObject::connectSlotsByName(this) 的功能。

这意味着：Qt 会自动帮你把 btnOpen 的 clicked 信号连接到 on_btnOpen_clicked 这个槽上
如果你手动又写了一遍 connect，在这个按钮被点击时，你的槽函数会执行两次！
一句话:只要函数名符合 on_控件名_信号名() 的格式，Qt 会自动帮你连上。
*/
    // connect(ui->btnOpen,&QPushButton::clicked,this,&MainWindow::on_btnOpen_clicked);

    qDebug()<<"FFmpeg Version: "<<av_version_info();

    //multiThread
    m_decodeThread = new DecodeThread();

}

MainWindow::~MainWindow()
{
    delete ui;
}

// 假设 demuxer 是 MainWindow 的成员变量： VideoDemuxer m_demuxer;

void MainWindow::on_btnOpen_clicked() {
    //single thread
    // QString fileName = QFileDialog::getOpenFileName(this, "Select Video");
    // if (fileName.isEmpty()) return;

    // if (m_demuxer.Open(fileName.toStdString().c_str())) {
    //     ui->objName->append("Open Success!");
    //     ui->objName->append("Resolution: " + QString::number(m_demuxer.getWidth()) + "x" + QString::number(m_demuxer.getHeight()));

    //     // 读取 10 帧测试一下
    //     AVPacket* pkt = av_packet_alloc(); // 1. 分配壳
    //     for(int i=0; i<10; i++) {
    //         if (m_demuxer.Read(pkt) == 0) {
    //             ui->objName->append(QString("Read Packet size=%1 pts=%2").arg(pkt->size).arg(pkt->pts));
    //             av_packet_unref(pkt); // 2. 释放内存 (记得！)
    //         } else {
    //             break;
    //         }
    //     }
    //     av_packet_free(&pkt); // 3. 释放壳
    // }

    //multi thread
    QString fileName = QFileDialog::getOpenFileName(this, "Select Video");

    m_decodeThread->open(fileName.toStdString());
}
