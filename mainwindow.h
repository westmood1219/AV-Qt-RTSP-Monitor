#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "VideoDemuxer.h"
#include "DecodeThread.h"
#include "rtsp_player.h"


QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // void on_btnOpen_clicked();
    // void on_rtspAddr_returnPressed();
    // void onFrameDecoded(QImage image);
    void onPlayerDoubleClicked(QWidget *w);
private:
    Ui::MainWindow *ui;

    bool isMaximizedState = false;
    QList<RTSPPlayer*> players;
};
#endif // MAINWINDOW_H
