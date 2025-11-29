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
    //before open the file
    avformat_network_init();//init 网络库

    //--RTSP 优化参数配置---
    AVDictionary* options = nullptr;

    // [网络层]
    av_dict_set(&options, "rtsp_transport", "tcp", 0); // 必选 TCP，稳
    av_dict_set(&options, "stimeout", "3000000", 0);   // 3秒超时

    // [缓冲区层] 核心！让 FFmpeg 只要收到数据包就立刻吐出来，不要存
    //所以不要buffer
    // av_dict_set(&options, "buffer_size", "1024000", 0); // 增大接收缓冲防丢包
    av_dict_set(&options, "max_delay", "100000", 0);    // 最大延迟 0.1秒

    // [探测层] 极速打开，不分析太久
    // av_dict_set(&options, "probesize", "102400", 0);    // 减少探测数据量
    // av_dict_set(&options, "analyzeduration", "100000", 0); // 减少分析时长
    //有了后面的 Lazy Init，这里用默认的也可以，不用刻意改太小

    // [标志位] 无缓冲
    av_dict_set(&options, "fflags", "nobuffer", 0);
    av_dict_set(&options, "allowed_media_types", "video", 0); // 只看视频

    //demuxer to get the pkt
    if(!m_demuxer->Open(m_url.c_str(),&options)){
        qDebug()<<"Open failed";
        av_dict_free(&options); // 失败记得释放
        return;
    }
    av_dict_free(&options); // 成功也释放

    qDebug()<<"Open success, starting decode loop...";
    //拿到上下文
    AVFormatContext *fmtCtx = m_demuxer->getFormatContext();

    //1 寻找视频流
    m_videoStreamIndex = av_find_best_stream(fmtCtx,AVMEDIA_TYPE_VIDEO,-1,-1,nullptr,0);
    if (m_videoStreamIndex<0){
        qDebug()<<"can't find video stream";
        return;
    }
    AVStream *videoStream = fmtCtx->streams[m_videoStreamIndex];

    //2 find decoder
    const AVCodec *codec = avcodec_find_decoder(videoStream->codecpar->codec_id);
    if(!codec){
        qDebug()<<"can't find decoder";
        return;
    }

    //3.create decoder context
    m_codecCtx = avcodec_alloc_context3(codec);

    //4.copy the paraments in the stream(resolution,bitrate,etc.) to the decoderContext
    avcodec_parameters_to_context(m_codecCtx,videoStream->codecpar);

    //5.open the decoder
    // 解码器开启 "低延迟" 模式
    m_codecCtx->flags |= AV_CODEC_FLAG_LOW_DELAY;
    // 如果是 H264，有些流可能需要 flags2
    if (codec->id == AV_CODEC_ID_H264) {
        m_codecCtx->flags2 |= AV_CODEC_FLAG2_FAST;
    }

    if(avcodec_open2(m_codecCtx,codec,nullptr)<0){
        qDebug()<<"can't open decoder";
        return;
    }

    qDebug()<<"open decoder successfully";

    //--------------Pepare for Swsformat(Software scale)-----------------
    AVFrame *pFrame = av_frame_alloc();//Store the decoded original YUV data
    AVFrame *pFrameRGB = av_frame_alloc();//Store converted RGB data
    AVPacket *pPacket = av_packet_alloc();//Store the compressed package read from the file

    //define output format:RGB24,width&height as same as original
    int outWidth = m_codecCtx->width;
    int outHeight = m_codecCtx->height;
    uint8_t *rgbBuffer = nullptr;
    m_swsCtx = nullptr;

    //===改用 "延迟初始化"，等真的拿到图片了再创建，防止 crash
    // enum AVPixelFormat outFormat = AV_PIX_FMT_RGB24;

    // //Allocate the buffer of pFrameRGB
    // int numBytes = av_image_get_buffer_size(outFormat,outWidth,outHeight,1);
    // uint8_t *buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
    // av_image_fill_arrays(pFrameRGB->data,pFrameRGB->linesize,buffer,outFormat,outWidth,outHeight,1);

    // //init SWS context(converter)
    // //@para:input width,height,format->output width,height,format->algorithm(BICUBIC is relatively balanced)
    // m_swsCtx = sws_getContext(m_codecCtx->width,m_codecCtx->height,m_codecCtx->pix_fmt,
    //                           outWidth,outHeight,outFormat,
    //                           SWS_BICUBIC,nullptr,nullptr,nullptr);

    //============Loop Decoding==================
    while(!m_isStop){
        //1.read a pkt from file(Demux)
        //VideoDemuxer里已经封装了read
        if(m_demuxer->Read(pPacket)<0){
            qDebug()<<"read error";
            break;
        }

        //only handle videoStream
        //2.Send packet to decoder
        int ret = avcodec_send_packet(m_codecCtx,pPacket);
        if(ret<0){
            qDebug()<<"error when send pkt";
            av_packet_unref(pPacket);
            continue;
        }

        //3.Receive frame in loop
        //Note: A Packet may demux multiple Frames, or it may not demux Frames, so you must use while
        while(ret>=0){
            ret = avcodec_receive_frame(m_codecCtx,pFrame);
            if(ret==AVERROR(EAGAIN) ||ret==AVERROR_EOF){
                //EAGAIN:need more packet to decoder a frame
                //EOF: end of file
                break;
            }else if(ret<0){
                qDebug()<<"decoder error";
                break;
            }

            //---[change11_29]Lazy Init (延迟初始化 SwsContext) ---
            // 只有当第一次拿到 Frame，或者分辨率发生变化时，才初始化
            if (m_swsCtx == nullptr ||
                pFrame->width != m_codecCtx->width ||
                pFrame->height != m_codecCtx->height){
                //释放旧的 (如果有
                if (m_swsCtx) sws_freeContext(m_swsCtx);
                if (rgbBuffer) av_free(rgbBuffer);

                //更新尺寸信息(以 pFrame 为准，因为 Stream 信息可能不准)
                m_codecCtx->width = pFrame->width;
                m_codecCtx->height = pFrame->height;
                outWidth = pFrame->width;
                outHeight = pFrame->height;
                enum AVPixelFormat outFormat = AV_PIX_FMT_RGB24;

                // 重新分配 RGB 内存
                int numBytes = av_image_get_buffer_size(outFormat, outWidth, outHeight, 1);
                rgbBuffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
                av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, rgbBuffer, outFormat, outWidth, outHeight, 1);

                // 创建转换器 (注意这里用 pFrame->format，绝对不会是 Unknown)
                m_swsCtx = sws_getContext(pFrame->width, pFrame->height, (enum AVPixelFormat)pFrame->format,
                                          outWidth, outHeight, outFormat,
                                          SWS_BICUBIC, nullptr, nullptr, nullptr);

                qDebug() << "SwsContext Initialized: " << outWidth << "x" << outHeight;

            }

            //convert
            if(m_swsCtx){

                //Coming here means that there is already a complete YUV image in pFrame!
                //此时 pFrame->data[0] 是 Y, data[1] 是 U, data[2] 是 V

                //==========convert YUV->RGB & deplay
                //4.format convert
                sws_scale(m_swsCtx,
                          (const uint8_t *const *)pFrame->data,pFrame->linesize,
                          0,m_codecCtx->height,
                          pFrameRGB->data,pFrameRGB->linesize);

                //5.构造QImage
                //Note:这里必须深拷贝,(QImage默认浅拷贝,不处理的话下一帧数据覆盖这里的时候会崩)
                //最简单的方法:直接构造QImage,并通过信号传值(Qt信号槽跨线程会自动copy)
                QImage img((uchar *)pFrameRGB->data[0],outWidth,outHeight,QImage::Format_RGB888);

                //6.send to UI(不要在子线程操作UI)
                emit sig_frameDecoded(img.copy());//.copy()确保所有权分离
            }
        }

        av_packet_unref(pPacket); // 释放包的引用

    }

    //=========内存清理========
    qDebug()<<"release resource";
    if (rgbBuffer) av_free(rgbBuffer);
    av_frame_free(&pFrameRGB);
    av_frame_free(&pFrame);
    av_packet_free(&pPacket);
    if (m_swsCtx) sws_freeContext(m_swsCtx);
    avcodec_free_context(&m_codecCtx);
    //fmtCtx由demuxer管理,这里不释放

}
