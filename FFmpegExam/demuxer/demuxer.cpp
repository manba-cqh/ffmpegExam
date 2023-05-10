#include <iostream>
using namespace std;
#define __STDC_CONSTANT_MACROS
extern "C"
{
#include "libavutil/imgutils.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include "libswresample/swresample.h"
}
int main()
{
	AVFormatContext* pFormatCtx;
	int             i, videoindex;
	AVCodecContext* pCodecCtx;
	const AVCodec* pCodec;
	AVFrame* pFrame, * pFrameYUV;
	uint8_t* out_buffer;
	AVPacket* packet;
	int             y_size;
	int             ret, got_picture;
	struct SwsContext* img_convert_ctx;

	char filepath[] = "test.mp4";

	//FILE *fp_yuv;
	int frame_cnt;

	avformat_network_init();//初始化网络链接
	pFormatCtx = avformat_alloc_context();//初始化AVFormatContext结构体类型变量——pFormatCtx

	if (avformat_open_input(&pFormatCtx, filepath, NULL, NULL) != 0) {
		//打开输入视频文件
		printf("Couldn't open input stream.\n");
		return -1;
	}
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
		//获取视频文件信息
		printf("Couldn't find stream information.\n");
		return -1;
	}
	videoindex = -1;
	AVCodecParameters* codecPar = nullptr;
	for (i = 0; i < pFormatCtx->nb_streams; i++) {
		//遍历streams数组，找到视频流在streams中的索引
		codecPar = pFormatCtx->streams[i]->codecpar;
		AVCodecID codecId = codecPar->codec_id;
		pCodec = avcodec_find_decoder(codecId);
		if (pCodec->type == AVMEDIA_TYPE_VIDEO) {
			videoindex = i;
			break;
		}
	}
	if (videoindex == -1) {
		printf("Didn't find a video stream.\n");
	}

	pCodecCtx = avcodec_alloc_context3(pCodec);
	ret = avcodec_parameters_to_context(pCodecCtx, codecPar);
	if (pCodec == NULL) {
		printf("Codec not found.\n");
		return -1;
	}
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		//打开解码器
		printf("Could not open codec.\n");
		return -1;
	}

	FILE* fp = fopen("output.txt", "wb+");

	fprintf(fp, "视频文件名：%s\n", filepath);
	fprintf(fp, "封装格式：%s\n", pFormatCtx->iformat->name);
	fprintf(fp, "码率：%d bps\n", pFormatCtx->bit_rate);
	fprintf(fp, "编码方式：%s\n", pCodec->name);
	fprintf(fp, "时长：%.2f s\n", (double)pFormatCtx->duration / 1000000);
	fprintf(fp, "宽*高：%d*%d\n", pCodecCtx->width, pCodecCtx->height);


	pFrame = av_frame_alloc();
	pFrameYUV = av_frame_alloc();
	out_buffer = (unsigned char*)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1));
	av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);
	packet = (AVPacket*)av_malloc(sizeof(AVPacket));
	//Output Info-----------------------------
	//printf("--------------- File Information ----------------\n");
	//av_dump_format(pFormatCtx, 0, filepath, 0);
	//printf("-------------------------------------------------\n");
	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
		pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);


	FILE* fp_264 = fopen("output.h264", "wb+");
	FILE* fp_yuv = fopen("output.yuv", "wb+");

	//unsigned char* dummy = NULL;   //输入的指针  
	//int dummy_len;
	//AVBitStreamFilterContext* bsfc = av_bitstream_filter_init("h264_mp4toannexb");
	//av_bitstream_filter_filter(bsfc, pCodecCtx, NULL, &dummy, &dummy_len, NULL, 0, 0);
	//fwrite(pCodecCtx->extradata, pCodecCtx->extradata_size, 1, fp_264);
	//av_bitstream_filter_close(bsfc);
	//free(dummy);

	frame_cnt = 0;
	while (av_read_frame(pFormatCtx, packet) >= 0) {
		//循环输入文件按一次一帧读取压缩数据到packet
		if (packet->stream_index == videoindex) {
			//char nal_start[] = { 0,0,0,1 };
			//fwrite(nal_start, 4, 1, fp_264);
			fwrite(packet->data, 1, packet->size, fp_264);
			fprintf(fp, "第%d帧解码前帧大小：%d Byte\n", frame_cnt, packet->size);
			if (avcodec_send_packet(pCodecCtx, packet) < 0)
			{
				continue;
			}

			if (avcodec_receive_frame(pCodecCtx, pFrame) < 0)
			{
				continue;
			}
			//从packet中解码一帧压缩数据，赋给pFrame
			fprintf(fp, "第%d帧解码后帧类型：", frame_cnt);
			switch (pFrame->pict_type)
			{
			case 0: fprintf(fp, "AV_PICTURE_TYPE_NONE\n"); break;
			case 1: fprintf(fp, "AV_PICTURE_TYPE_I\n"); break;
			case 2: fprintf(fp, "AV_PICTURE_TYPE_P\n"); break;
			case 3: fprintf(fp, "AV_PICTURE_TYPE_B\n"); break;
			case 4: fprintf(fp, "AV_PICTURE_TYPE_S\n"); break;
			case 5: fprintf(fp, "AV_PICTURE_TYPE_SI\n"); break;
			case 6: fprintf(fp, "AV_PICTURE_TYPE_SP\n"); break;
			case 7: fprintf(fp, "AV_PICTURE_TYPE_BI\n"); break;

			}
			if (ret < 0) {
				printf("Decode Error.\n");
				return -1;
			}
			sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0,
				pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);
			//printf("Decoded frame index:%d\n", frame_cnt);
			fwrite(pFrameYUV->data[0], 1, pCodecCtx->width* pCodecCtx->height, fp_yuv);//Y
			fwrite(pFrameYUV->data[1], 1, pCodecCtx->width* pCodecCtx->height / 4, fp_yuv);//U
			fwrite(pFrameYUV->data[2], 1, pCodecCtx->width* pCodecCtx->height / 4, fp_yuv);//V
			frame_cnt++;
		}
		av_packet_unref(packet);
		av_freep(packet);
	}
	fclose(fp);
	fclose(fp_264);
	fclose(fp_yuv);
	return 0;
}