extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
}

#define SRCFILE "output.yuv" 
#define DSTFILE "out.yuv" 

//sws_scale用法，改变yuv文件分辨率
int main()
{
	// 设定原始 YUV 的长宽 
	const int in_width = 1920;
	const int in_height = 1080;
	// 设定目的 YUV 的长宽
	const int out_width = 640;
	const int out_height = 1080;

	const int read_size = in_width * in_height * 3 / 2;
	const int write_size = out_width * out_height * 3 / 2;
	struct SwsContext* img_convert_ctx;
	uint8_t* inbuf[4];
	uint8_t* outbuf[4];
	int inlinesize[4] = { in_width, in_width / 2, in_width / 2, 0 };
	int outlinesize[4] = { out_width, out_width / 2, out_width / 2, 0 };

	//避免栈溢出
	uint8_t *in = new uint8_t[in_width * in_height * 3 >> 1];
	uint8_t *out = new uint8_t[out_width * out_height * 3 >> 1];

	FILE* fin = fopen(SRCFILE, "rb");
	FILE* fout = fopen(DSTFILE, "wb");

	if (fin == NULL) {
		printf("open input file %s error.\n", SRCFILE);
		return -1;
	}

	if (fout == NULL) {
		printf("open output file %s error.\n", DSTFILE);
		return -1;
	}

	inbuf[0] = (uint8_t*)malloc(in_width * in_height);
	inbuf[1] = (uint8_t*)malloc(in_width * in_height >> 2);
	inbuf[2] = (uint8_t*)malloc(in_width * in_height >> 2);
	inbuf[3] = NULL;

	outbuf[0] = (uint8_t*)malloc(out_width * out_height);
	outbuf[1] = (uint8_t*)malloc(out_width * out_height >> 2);
	outbuf[2] = (uint8_t*)malloc(out_width * out_height >> 2);
	outbuf[3] = NULL;

	// ********* Initialize software scaling ********* 
	// ********* sws_getContext ********************** 
	img_convert_ctx = sws_getContext(in_width, in_height, AV_PIX_FMT_YUV420P,
		out_width, out_height, AV_PIX_FMT_YUV420P, SWS_POINT,
		NULL, NULL, NULL);
	if (img_convert_ctx == NULL) {
		fprintf(stderr, "Cannot initialize the conversion context!\n");
		return -1;
	}

	fread(in, 1, read_size, fin);

	memcpy(inbuf[0], in, in_width * in_height);
	memcpy(inbuf[1], in + in_width * in_height, in_width * in_height >> 2);
	memcpy(inbuf[2], in + (in_width * in_height * 5 >> 2), in_width * in_height >> 2);

	// ********* 主要的 function ****** 
	// ********* sws_scale ************ 
	sws_scale(img_convert_ctx, inbuf, inlinesize,
		0, in_height, outbuf, outlinesize);

	memcpy(out, outbuf[0], out_width * out_height);
	memcpy(out + out_width * out_height, outbuf[1], out_width * out_height >> 2);
	memcpy(out + (out_width * out_height * 5 >> 2), outbuf[2], out_width * out_height >> 2);

	fwrite(out, 1, write_size, fout);

	// ********* 结束的 function ******* 
	// ********* sws_freeContext ******* 
	sws_freeContext(img_convert_ctx);

	fclose(fin);
	fclose(fout);

	delete in;
	delete out;

	return 0;
}

//解封装
/*#include <iostream>
using namespace std;
#define __STDC_CONSTANT_MACROS
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
}
int main()
{
	AVFormatContext* pFormatCtx;
	int             i, videoindex;
	AVCodecContext* pCodecCtx;
	AVCodec* pCodec;
	AVFrame* pFrame, * pFrameYUV;
	uint8_t* out_buffer;
	AVPacket* packet;
	int             y_size;
	int             ret, got_picture;
	struct SwsContext* img_convert_ctx;

	char filepath[] = "test.avi";

	//FILE *fp_yuv;
	int frame_cnt;

	av_register_all();//注册所有组件
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
	for (i = 0; i < pFormatCtx->nb_streams; i++) {
		//遍历streams数组，找到视频流在streams中的索引
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoindex = i;
			break;
		}
	}
	if (videoindex == -1) {
		printf("Didn't find a video stream.\n");
	}

	pCodecCtx = pFormatCtx->streams[videoindex]->codec;//将视频流中的AVCodecContext赋给pCodecCtx
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);//查找解码器
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
	fprintf(fp, "编码方式：%s\n", pFormatCtx->streams[videoindex]->codec->codec->name);
	fprintf(fp, "时长：%.2f s\n", (double)pFormatCtx->duration / 1000000);
	fprintf(fp, "宽*高：%d*%d\n", pFormatCtx->streams[videoindex]->codec->width, pFormatCtx->streams[videoindex]->codec->height);


	pFrame = av_frame_alloc();
	pFrameYUV = av_frame_alloc();
	out_buffer = (unsigned char*)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
	avpicture_fill((AVPicture*)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);
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
			ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
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
			if (got_picture) {
				sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0,
					pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);
				//printf("Decoded frame index:%d\n", frame_cnt);
				fwrite(pFrameYUV->data[0], 1, pCodecCtx->width * pCodecCtx->height, fp_yuv);//Y
				fwrite(pFrameYUV->data[1], 1, pCodecCtx->width * pCodecCtx->height / 4, fp_yuv);//U
				fwrite(pFrameYUV->data[2], 1, pCodecCtx->width * pCodecCtx->height / 4, fp_yuv);//V
				frame_cnt++;
			}
		}
		av_free_packet(packet);
	}
	fclose(fp);
	fclose(fp_264);
	fclose(fp_yuv);
	return 0;
}
*/