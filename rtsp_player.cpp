#include "rtsp_player.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QLayout>
#include <QMenu>
#include <QContextMenuEvent>

RTSPPlayer::RTSPPlayer(QWidget *parent)
    : QWidget{parent}
{

    isMax = false;
    // // 1. 初始化 UI 控件
    m_videoLabel = new QLabel(this);
    m_videoLabel->setAlignment(Qt::AlignCenter);
    m_videoLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored); // 允许缩放
    m_videoLabel->setScaledContents(true); // 【关键优化】开启 QLabel 自动缩放

    // // 2. 设置布局 (让 Label 填充满整个 Widget)
    QGridLayout *layout = new QGridLayout(this);
    layout->addWidget(m_videoLabel);
    this->setLayout(layout);

    m_btnFullScreen = new QPushButton("⛶",this);
    m_btnFullScreen->setFixedSize(36,36);
    m_btnFullScreen->setVisible(false);
    m_btnFullScreen->setStyleSheet(
        "QPushButton {"
        "background-color: rgba(0,0,0,150);"
        "color:white;"
        "border:none;"
        "}"
        );


    //初始化线程 (线程对象必须在 UI 线程创建)
    m_decodeThread = new DecodeThread(this);

    //连接信号槽：线程发送图片 -> 当前组件接收并更新 UI
    connect(m_decodeThread, &DecodeThread::sig_frameDecoded,
            this, &RTSPPlayer::updateFrame, Qt::QueuedConnection);
    // 必须使用 Qt::QueuedConnection，确保 UI 更新操作是在 UI 线程执行。
    connect(m_btnFullScreen, &QPushButton::clicked, this, &RTSPPlayer::onm_btnFullScreenClicked);
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

void RTSPPlayer::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event);//避免未使用的参数警告

    //鼠标进入区域,显示按钮
    if(m_btnFullScreen)
    {
        //重定位按钮(确保在右上角)
        m_btnFullScreen->move(this->width()-m_btnFullScreen->width()-5,5);
        m_btnFullScreen->setVisible(true);
    }
    QWidget::enterEvent(event);
}

void RTSPPlayer::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    if(m_btnFullScreen)
    {
        m_btnFullScreen->setVisible(false);
    }
    QWidget::leaveEvent(event);
}

void RTSPPlayer::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event);

    emit sig_doubleClick(this);
}

void RTSPPlayer::onm_btnFullScreenClicked()
{
    emit sig_doubleClick(this);
}

void RTSPPlayer::contextMenuEvent(QContextMenuEvent *event)
{
    //创建菜单对象(栈上分配即可,用完即毁,无需new)
    QMenu menu(this);

    //创建截图动作
    QAction *actionSnapshot = menu.addAction("📸 截图");

    //创建"全屏"动作
    QAction *actionFullScreen=menu.addAction("⛶ 全屏");

    //连接信号与槽
    connect(actionSnapshot,&QAction::triggered,this,&RTSPPlayer::snapshot);

    //服用全屏逻辑
    connect(actionFullScreen,&QAction::triggered,[this](){
        emit sig_doubleClick(this);
    });

    //显示菜单
    // event->globalPos() 告诉菜单应该出现在鼠标当前的位置
    menu.exec(event->globalPos());

}

void RTSPPlayer::snapshot()
{
    if(m_currentImage.isNull()){
        qDebug()<<"无画面,无法截图";
        return;
    }

    //确保截图路径存在
    QString savePath = "E:/AV/snapshots";
    QDir dir(savePath);
    if(!dir.exists()) dir.mkpath(".");

    //文件名:路径+"/"+日期+格式
    QString fileName = savePath+"/"+
                       QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz")+".jpg";

    if(m_currentImage.save(fileName,"JPG"))
    {
        qDebug()<<"截图已保存:  "<<fileName;
    }else{
        qDebug()<<"截图保存失败";
    }
}



void RTSPPlayer::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    if (m_btnFullScreen) {
        // 确保按钮始终位于当前 Widget 的右上角，并留出 5 像素边距
        m_btnFullScreen->move(width() - m_btnFullScreen->width() - 5, 5);
    }
    // 必须调用基类的 resizeEvent
    QWidget::resizeEvent(event);
}

void RTSPPlayer::updateFrame(QImage image)
{
    if(m_videoLabel && !image.isNull()){

        m_currentImage = image;

        //缩放图片以适应Label大小
        QPixmap pixmap = QPixmap::fromImage(image);
        // m_videoLabel->setPixmap(pixmap.scaled(m_videoLabel->size(),Qt::KeepAspectRatio));    }
        //前面 setScaledContents(true) 了，Qt 底层会用 GPU 或优化算法自适应大小
        m_videoLabel->setPixmap(pixmap);
    }
}

void RTSPPlayer::mousePressEvent(QMouseEvent *event){
    emit sig_clicked(this);
    QWidget::mousePressEvent(event);
}


