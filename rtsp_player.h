#ifndef RTSP_PLAYER_H
#define RTSP_PLAYER_H

#include <QWidget>
#include <QLabel>
#include <QGridLayout>
#include "DecodeThread.h"

class RTSPPlayer : public QWidget
{
    Q_OBJECT
public:
    explicit RTSPPlayer(QWidget *parent = nullptr);
    ~RTSPPlayer();

    //对外接口:播放
    void play(const QString& url);

    //stop
    void stop();

// protected:
//     //重写绘画事件,替代QLabel
//     void paintEvent(QPaintEvent *event) override;

private slots:
    // //接收线程发来的图片
    // void slotUpdateImage(const QImage& image);
    // 接收线程发来的图片并更新UI
    void updateFrame(QImage image);

private:
    DecodeThread* m_decodeThread = nullptr;//管理解码 线程
    // QImage m_currentImage;// 保持当前最新一帧
    QLabel *m_videoLabel = nullptr;//用于显示画面的Label

signals:
};

#endif // RTSP_PLAYER_H
