#ifndef VIDEODEMUXER_H
#define VIDEODEMUXER_H

#pragma once
extern "C"{
#include <libavformat/avformat.h>
}
#include <string>

class VideoDemuxer{
public:
    VideoDemuxer();
    ~VideoDemuxer();

    //打开文件 ,成功返回true
    bool Open(const char* url, AVDictionary** options);
    //读取一帧数据
    //返回0true
    //参数:pkt是由调用者分配好并传进来的空包
    int Read(AVPacket* pkt);
    void Close();
    int getWidth() const;
    int getHeight() const;
    // 声明获取 FormatContext 的接口
    AVFormatContext* getFormatContext();

private:
    AVFormatContext* fmt_ctx = nullptr;
    int video_stream_index = -1;
    int width = 0;
    int height = 0;
};

#endif // VIDEODEMUXER_H
