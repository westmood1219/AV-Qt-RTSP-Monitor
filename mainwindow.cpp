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

    if(ui->rtspAddr){
        // 信号：必须写 (&QLineEdit::returnPressed)
        //connect(ui->rtspAddr, static_cast<void (QLineEdit::*)()>(&QLineEdit::returnPressed), this, &MainWindow::on_rtspAddr_returnPressed);
        connect(ui->rtspAddr, SIGNAL(returnPressed()),
                this, SLOT(on_rtspAddr_returnPressed()));
    }

    //multiThread
    m_decodeThread = new DecodeThread();

    connect(m_decodeThread,&DecodeThread::sig_frameDecoded,this,&MainWindow::onFrameDecoded);

}

MainWindow::~MainWindow()
{
    delete ui;
}

// 假设 demuxer 是 MainWindow 的成员变量： VideoDemuxer m_demuxer;

void MainWindow::on_btnOpen_clicked() {
    //multi thread
    QString fileName = QFileDialog::getOpenFileName(this, "Select Video");

    m_decodeThread->open(fileName.toStdString());
}

void MainWindow::on_rtspAddr_returnPressed() {
    // 获取输入框中的文本
    QString url = ui->rtspAddr->text().trimmed(); // .trimmed() 移除前后空格

    if (url.isEmpty()) {
        QMessageBox::warning(this, "错误", "RTSP 地址不能为空！");
        return;
    }

    // 简单的 RTSP 格式检查
    if (!url.startsWith("rtsp://", Qt::CaseInsensitive)) {
        QMessageBox::warning(this, "错误", "请输入有效的 RTSP 地址（例如 rtsp://...）");
        return;
    }

    // 使用 RTSP URL 启动解码
    m_decodeThread->open(url.toStdString());
}


void MainWindow::onFrameDecoded(QImage image){
    //把图片显示在Label上
    ui->label->setPixmap(QPixmap::fromImage(image).scaled(ui->label->size(),Qt::KeepAspectRatio));
}
