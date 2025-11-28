#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "VideoDemuxer.h"
#include "DecodeThread.h"


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
    void on_btnOpen_clicked();
    void onFrameDecoded(QImage image);
private:
    Ui::MainWindow *ui;
    DecodeThread *m_decodeThread;
    VideoDemuxer m_demuxer;
};
#endif // MAINWINDOW_H
