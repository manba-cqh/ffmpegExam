extern "C" {
#include "libavutil/opt.h"
#include "libavutil/channel_layout.h"
#include "libavutil/common.h"
#include "libavutil/mathematics.h"
#include "libavutil/samplefmt.h"
#include "libavutil/time.h"
#include "libavutil/fifo.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
}

#include <iostream>
#include <string>
#include <memory>

AVFormatContext *inputContext = nullptr;
AVFormatContext *outputContext;
AVPacket* avPacket = nullptr;
int videoStreamIndex = -1;
int audioStreamIndex = -1;

int OpenInput(const std::string url)
{
    inputContext = avformat_alloc_context();
    if (inputContext == nullptr) {
        return -1;
    }

    int ret = avformat_open_input(&inputContext, url.c_str(), nullptr, nullptr);
    if (ret != 0) {
        return ret;
    }

    ret = avformat_find_stream_info(inputContext, nullptr);
    if (ret < 0) {
        return -1;
    }

    return 0;
}

int OpenOutput(const std::string url)
{
    int ret = avformat_alloc_output_context2(&outputContext, nullptr, "mpegts", url.c_str());
    if (ret < 0) {
        return ret;
    }

    ret = avio_open2(&outputContext->pb, url.c_str(), AVIO_FLAG_READ_WRITE, nullptr, nullptr);
    if (ret < 0) {
        return ret;
    }

    for (int i = 0; i < inputContext->nb_streams; i++) {
        AVCodecID codecId = inputContext->streams[i]->codecpar->codec_id;
        const AVCodec* avcodec = avcodec_find_decoder(codecId);
        if (avcodec == nullptr) {
            continue;
        }
        if (avcodec->type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
        }
        else {
            continue;
        }

        AVCodecContext* codexCtx = avcodec_alloc_context3(avcodec);
        ret = avcodec_parameters_to_context(codexCtx, inputContext->streams[i]->codecpar);
        if (ret < 0) {
            return ret;
        }

        AVStream* stream = avformat_new_stream(outputContext, avcodec);
        ret = avcodec_parameters_from_context(stream->codecpar, codexCtx);
        if (ret < 0) {
            return ret;
        }

        avformat_write_header(outputContext, nullptr);
    }

    return 0;
}

int ReadPacketFromSource()
{
    return av_read_frame(inputContext, avPacket);
}

void av_package_rescale_ts(AVPacket *packet, AVRational src_tb, AVRational dst_tb)
{
    if (packet->pts != AV_NOPTS_VALUE) {
        packet->pts = av_rescale_q(packet->pts, src_tb, dst_tb);
    }

    if (packet->dts != AV_NOPTS_VALUE) {
        packet->dts = av_rescale_q(packet->dts, src_tb, dst_tb);
    }

    if (packet->duration > 0) {
        packet->duration = av_rescale_q(packet->duration, src_tb, dst_tb);
    }

}

int WritePackage()
{
    if (avPacket->stream_index != videoStreamIndex) {
        return -1;
    }
    auto inputStream = inputContext->streams[avPacket->stream_index];
    auto outputStream = outputContext->streams[0];
    av_package_rescale_ts(avPacket, inputStream->time_base, outputStream->time_base);
    // 这里的avPacket->stream_index需要重新设置，根据outputContext的stream设置，现在还只支持写一个流，后续再研究
    avPacket->stream_index = 0;
    int ret = av_interleaved_write_frame(outputContext, avPacket);
    return ret;
}

void Init()
{
    avPacket = av_packet_alloc();
}

int main()
{
    Init();

    OpenInput("rtmp://172.18.220.155/live/livestream");
    OpenOutput("D:/text.ts");
    while (1) {
        ReadPacketFromSource();
        if (avPacket) {
            int ret = WritePackage();
            if (ret == 0) {
                std::cout << "write success\n";
            }
            else {
                std::cout << "write fail: " << ret << std::endl;
            }
        }
    }

    return 0;
}