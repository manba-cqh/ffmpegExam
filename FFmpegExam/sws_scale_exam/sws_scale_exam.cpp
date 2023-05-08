#include <cstdint>

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
}

#define SRCFILE "test.yuv" 
#define DSTFILE "output.yuv" 

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
	uint8_t* in = new uint8_t[in_width * in_height * 3 >> 1];
	uint8_t* out = new uint8_t[out_width * out_height * 3 >> 1];

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