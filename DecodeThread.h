#ifndef DECODETHREAD_H
#define DECODETHREAD_H

#include <QThread>
#include <QImage>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}
#include "VideoDemuxer.h"

class DecodeThread : public QThread{
    Q_OBJECT;
signals:
    //signal: notify the UI thread to render the image
    void sig_frameDecoded(QImage image);
public:
    explicit DecodeThread(QObject *parent = nullptr);
    ~DecodeThread();

    void open(const std::string& url);
    //incline 内联函数
    void setUrl(const std::string& url){m_url = url;};//封装url设置
    void stop(){m_isStop=true;}// 安全停止标记
protected:
    //core:override run method
    //Only the code here will run in a new thread
    void run() override;

private:
    std::string m_url;
    VideoDemuxer *m_demuxer;
    std::atomic<bool> m_isStop = false;//使用原子变量确保多线程可见性
    //decoder
    AVCodecContext *m_codecCtx = nullptr;//解码器上下文(PKT2Frame)
    SwsContext *m_swsCtx = nullptr;      //格式转换上下文(YUV2RGB)
    int m_videoStreamIndex = -1;         //视频流的索引

};



#endif // DECODETHREAD_H
