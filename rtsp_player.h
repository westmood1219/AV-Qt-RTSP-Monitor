#ifndef RTSP_PLAYER_H
#define RTSP_PLAYER_H

#include <QWidget>
#include <QLabel>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include<QEvent>
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

protected:
    //[重写这三个事件
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event);
    void onm_btnFullScreenClicked();

    void contextMenuEvent(QContextMenuEvent *event) override;
    void snapshot();

private slots:
    // 接收线程发来的图片并更新UI
    void updateFrame(QImage image);

private:
    //是否最大化
    bool isMax;

    DecodeThread* m_decodeThread = nullptr;//管理解码 线程
    QLabel *m_videoLabel = nullptr;//用于显示画面的Label
    QPushButton *m_btnFullScreen = nullptr;//全屏按钮

    QImage m_currentImage;//保存当前帧


signals:
    void sig_doubleClick(QWidget *w);
};

#endif // RTSP_PLAYER_H
