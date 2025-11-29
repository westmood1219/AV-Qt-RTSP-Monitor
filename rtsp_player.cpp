#include "rtsp_player.h"
#include <QDebug>
#include <QLayout>

RTSPPlayer::RTSPPlayer(QWidget *parent)
    : QWidget{parent}
{
    // 1. 初始化 UI 控件
    m_videoLabel = new QLabel(this);
    m_videoLabel->setAlignment(Qt::AlignCenter);
    m_videoLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored); // 允许缩放

    // 2. 设置布局 (让 Label 填充满整个 Widget)
    QGridLayout *layout = new QGridLayout(this);
    layout->addWidget(m_videoLabel);
    this->setLayout(layout);

    // 3. 初始化线程 (注意：线程对象必须在 UI 线程创建)
    m_decodeThread = new DecodeThread(this);

    // 4. 连接信号槽：线程发送图片 -> 当前组件接收并更新 UI
    connect(m_decodeThread, &DecodeThread::sig_frameDecoded,
            this, &RTSPPlayer::updateFrame, Qt::QueuedConnection);
    // 【关键】：必须使用 Qt::QueuedConnection，确保 UI 更新操作是在 UI 线程执行。

}

RTSPPlayer::~RTSPPlayer()
{
    stop(); // 析构时必须安全关闭线程
    delete m_decodeThread;
}

void RTSPPlayer::play(const QString &url)
{
    stop(); // 播放前先停止旧的线程

    m_videoLabel->setText("Connecting..."); // 提示用户正在连接

    // 1. 设置 URL
    m_decodeThread->setUrl(url.toStdString());

    // 2. 启动线程
    m_decodeThread->start();
    qDebug() << "RTSPPlayer playing:" << url;
}

void RTSPPlayer::stop()
{
    if(m_decodeThread->isRunning()){
        m_decodeThread->stop();//设置停止标记
        m_decodeThread->quit();//退出线程的事件循环
        m_decodeThread->wait();//等待线程安全退出
        qDebug()<<"RTSPPlayer stopped successfully";
    }
}



void RTSPPlayer::updateFrame(QImage image)
{
    if(m_videoLabel && !image.isNull()){
        //缩放图片以适应Label大小
        QPixmap pixmap = QPixmap::fromImage(image);
        m_videoLabel->setPixmap(pixmap.scaled(m_videoLabel->size(),Qt::KeepAspectRatio));    }
}


