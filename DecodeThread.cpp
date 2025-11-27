#include "DecodeThread.h"
#include <QDebug>


DecodeThread::DecodeThread(QObject *parent)
{
    m_isStop = false;
    m_demuxer = new VideoDemuxer();
}

DecodeThread::~DecodeThread()
{
    m_isStop = true;
    requestInterruption();
    wait();
    m_demuxer->Close();
}

void DecodeThread::open(const std::string &url)
{
    m_url = url;
    start();
}

void DecodeThread::run()
{
    if(!m_demuxer->Open(m_url.c_str())){
        qDebug()<<"Open failed";
        return;
    }

    qDebug()<<"Open success, starting decode loop...";

    //预先分配好内存，不要在死循环里分配
    AVPacket* pkt = av_packet_alloc();

    while(!isInterruptionRequested()&&!m_isStop){
        int ret = m_demuxer->Read(pkt);

        if(ret ==0){
            //解码
            qDebug()<<"Read a packet,Size: "<<pkt->size;
            av_packet_unref(pkt);
        }else{
            //读完了/出错了,稍微睡一会避免 CPU 100% 空转
            qDebug()<<"sleeping";
            msleep(10);

            continue;
        }
    }
    av_packet_free(&pkt);
    m_demuxer->Close();
    qDebug()<<"Thread finished";
}
