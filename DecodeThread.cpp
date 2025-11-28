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
    //demuxer to get the pkt
    if(!m_demuxer->Open(m_url.c_str())){
        qDebug()<<"Open failed";
        return;
    }

    qDebug()<<"Open success, starting decode loop...";

    //give the pkt a memory frame
    //预先分配好内存，不要在死循环里分配
    // AVPacket* pkt = av_packet_alloc();

    // while(!isInterruptionRequested()&&!m_isStop){
    //     int ret = m_demuxer->Read(pkt);

    //     if(ret ==0){
    //         //解码
    //         // qDebug()<<"Read a packet,Size: "<<pkt->size;
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
            enum AVPixelFormat outFormat = AV_PIX_FMT_RGB24;

            //Allocate the buffer of pFrameRGB
            int numBytes = av_image_get_buffer_size(outFormat,outWidth,outHeight,1);
            uint8_t *buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
            av_image_fill_arrays(pFrameRGB->data,pFrameRGB->linesize,buffer,outFormat,outWidth,outHeight,1);

            //init SWS context(converter)
            //@para:input width,height,format->output width,height,format->algorithm(BICUBIC is relatively balanced)
            m_swsCtx = sws_getContext(m_codecCtx->width,m_codecCtx->height,m_codecCtx->pix_fmt,
                                      outWidth,outHeight,outFormat,
                                      SWS_BICUBIC,nullptr,nullptr,nullptr);

            //============Loop Decoding==================
            while(!m_isStop){
                //1.read a pkt from file(Demux)
                if(av_read_frame(fmtCtx,pPacket)<0){
                    break;
                }

                //only handle videoStream
                if(pPacket->stream_index == m_videoStreamIndex){
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

                        //控制一下速度,不然瞬间播完(粗略模拟30fps)
                    QThread::msleep(33);//1000ms/30

                    }

                }

                av_packet_unref(pPacket); // 释放包的引用

            }

            //=========内存清理========
            qDebug()<<"release resource";
            av_free(buffer);
            av_frame_free(&pFrameRGB);
            av_frame_free(&pFrame);
            av_packet_free(&pPacket);
            sws_freeContext(m_swsCtx);
            avcodec_free_context(&m_codecCtx);
            //fmtCtx由demuxer管理,这里不释放



    //         av_packet_unref(pkt);
    //     }else{
    //         //读完了/出错了,稍微睡一会避免 CPU 100% 空转
    //         qDebug()<<"sleeping";
    //         msleep(10);

    //         continue;
    //     }
    // }
    // av_packet_free(&pkt);
    // m_demuxer->Close();
    // qDebug()<<"Thread finished";
}
