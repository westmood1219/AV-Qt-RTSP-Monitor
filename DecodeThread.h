#ifndef DECODETHREAD_H
#define DECODETHREAD_H

#include <QThread>
#include <QImage>
#include "VideoDemuxer.h"

class DecodeThread : public QThread{
    Q_OBJECT;
public:
    explicit DecodeThread(QObject *parent = nullptr);
    ~DecodeThread();

    void open(const std::string& url);
protected:
    //core:override run method
    //Only the code here will run in a new thread
    void run() override;

private:
    std::string m_url;
    VideoDemuxer *m_demuxer;
    bool m_isStop;
};



#endif // DECODETHREAD_H
