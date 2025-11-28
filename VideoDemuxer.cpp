#include "VideoDemuxer.h"
#include <iostream>

VideoDemuxer::VideoDemuxer() {
    // 可以在这里做网络初始化 avformat_network_init() (新版本其实不需要了，但也无妨)
}

VideoDemuxer::~VideoDemuxer() {
    Close();
}

bool VideoDemuxer::Open(const char* url) {
    Close(); // 防止重复打开，先清理

    // 1. Open Context
    // options 传 NULL，使用默认参数
    int ret = avformat_open_input(&fmt_ctx, url, nullptr, nullptr);
    if (ret < 0) {
        char errbuf[1024];
        av_strerror(ret, errbuf, sizeof(errbuf));
        std::cerr << "Open Error: " << errbuf << std::endl;
        return false;
    }

    // 2. Find Stream Info
    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        std::cerr << "Find Stream Info Error" << std::endl;
        return false;
    }

    // 3. Find Video Stream
    video_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (video_stream_index < 0) {
        std::cerr << "No Video Stream" << std::endl;
        return false;
    }

    // 获取宽高等参数方便后续使用
    AVCodecParameters* par = fmt_ctx->streams[video_stream_index]->codecpar;
    width = par->width;
    height = par->height;

    std::cout << "Opened: " << url << " (" << width << "x" << height << ")" << std::endl;
    return true;
}

int VideoDemuxer::Read(AVPacket* pkt) {
    if (!fmt_ctx) return -1;

    int ret;
    while ((ret = av_read_frame(fmt_ctx, pkt)) >= 0) {
        // 如果读到的是视频流，就返回成功
        if (pkt->stream_index == video_stream_index) {
            return 0; // Success
        }

        // 如果不是视频流（比如音频），我们这里选择丢弃，释放引用，继续读下一帧
        // 注意：如果不 unref，内存会爆
        av_packet_unref(pkt);
    }
    av_packet_unref(pkt);
    return ret; // 返回错误码或 EOF
}

void VideoDemuxer::Close() {
    if (fmt_ctx) {
        avformat_close_input(&fmt_ctx);
        fmt_ctx = nullptr;
    }
    video_stream_index = -1;
}

int VideoDemuxer::getWidth() const { return width; }
int VideoDemuxer::getHeight() const { return height; }

AVFormatContext *VideoDemuxer::getFormatContext()
{
    return fmt_ctx;
}
