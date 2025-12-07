#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QMessageBox>
#include <qpushbutton.h>
#include "ui_mainwindow.h"
#include <QSplitter>
#include <QStandardItem>
#include <QListView>

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

    // 1. 准备布局：使用 QSplitter (分割器)，这样用户可以拖动调整左右大小
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(splitter);

    // 2. 左边：列表 (View)
    QListView *cameraListView = new QListView(this);
    splitter->addWidget(cameraListView);
    splitter->setStretchFactor(0, 1);

    // 3. 右边：视频墙 (我们封装好的新类)
    VideoWall *videoWall = new VideoWall(this);
    splitter->addWidget(videoWall);

    splitter->setStretchFactor(1, 32);

    // 4. Model/View

    // A. 创建 Model (标准项模型)
    QStandardItemModel *model = new QStandardItemModel(this);

    // B. 模拟数据 (真实项目中这里应该是读取数据库或配置文件)
    struct CameraInfo {
        QString name;
        QString url;
    };
    QList<CameraInfo> cameras = {
        {"大门监控", "rtsp://10.163.18.72:8090/h264_ulaw.sdp"},
        {"大厅监控", "rtsp://10.163.18.72:8090/h264_ulaw.sdp"},
        {"走廊监控", "rtsp://10.163.18.72:8090/h264_ulaw.sdp"},
        {"测试视频", "F:/ffmpegTraining/test.mp4"}
    };

    // C. 填充 Model
    for (const auto& cam : cameras) {
        QStandardItem *item = new QStandardItem(cam.name); // 显示文本

        item->setData(cam.url, Qt::UserRole);

        model->appendRow(item);
    }

    // D. View 绑定 Model
    cameraListView->setModel(model);

    // ----------------------------------------------------
    // 5. 交互逻辑
    // ----------------------------------------------------

    // 当左边列表被点击 -> 告诉右边视频墙改变播放源
    connect(cameraListView, &QListView::clicked, [=](const QModelIndex &index){
        // 从 Model 中取出藏好的 URL
        // index.data(Qt::UserRole) 返回的是 QVariant，需要转成 String
        QString url = index.data(Qt::UserRole).toString();

        qDebug() << "更改了摄像头：" << index.data(Qt::DisplayRole).toString() << "URL:" << url;

        // 调用 VideoWall 播放
        videoWall->playUrl(url);
    });

}

MainWindow::~MainWindow()
{
    delete ui;
}
