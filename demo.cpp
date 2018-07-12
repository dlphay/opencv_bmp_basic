#include <Windows.h>   
#include<iostream>
#include<time.h>

// opencv
#include <opencv/cv.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>//图像处理 
#include <opencv/cvaux.h> 
#include <opencv2/opencv.hpp>

//#include <opencv2/imgproc.hpp>

using namespace std;
using namespace cv;

// 全局变量
unsigned char* pBmpBuf;
int bmpWidth; 
int bmpHeight;
RGBQUAD* pColorTable;
int biBitCount;
int strick = 0;



INT BayerFilter(BYTE *pSrc, DWORD width, DWORD height, BYTE *pDest)
{
	BYTE *pDestSave = pDest;
	DWORD widthSave = width;
	DWORD heightSave = height;

	if (pSrc == NULL || pDest == NULL)
		return FALSE;

	__asm
	{
		mov         ebx, pDest; pDest
		mov         eax, height; height
		mov         edx, width; width(i)
		sub         eax, 2; height - 2
		mul         edx; edx:eax = width * (height - 2)
		mov         ecx, eax; ecx = width * (height - 2)
		shl         eax, 1; width * (height - 2) * 2
		add         eax, ecx; width * (height - 2) * 3
		add         ebx, eax; pDest + width * (height - 2) * 3
		mov         edx, width; width(i)
		mov         edi, edx; width
		mov         esi, edx; width
		mov			eax, pSrc; pSource
		dec         esi; width - 1
		mov         width, esi; width - 1
		dec         height; height - 1
		//////////////////////////////////////////////////////////////////////////////////////
		; eax: source pointer
		; ebx: destination pointer, start at the end address, actually top of the image
		; ecx: available
		; edx: available
		; edi: width
		; esi: horizontal counter
		/////////////////////////////////////////////////////////////////////////////////////	
		Bayer3RGGBToRgb24Invert_odd_odd :
		mov         cl, [eax + 1]; Green type 1
			mov         dl, [eax]; Red
			add         cl, [eax + edi]; Green type 2
			mov[ebx + 2], dl; Write back red
			rcr         cl, 1; Real green
			mov         dl, [eax + edi + 1]; Blue
			mov[ebx + 1], cl; Write back green
			mov[ebx], dl; Write back blue
			dec         esi; placed there for pairing
			jnz         Bayer3RGGBToRgb24Invert_odd_even
			; The line is done
			inc         eax
			add         ebx, 3
			jmp         Bayer3RGGBToRgb24Invert_even_setup
			Bayer3RGGBToRgb24Invert_odd_even :
		mov         cl, [eax + 1]; green type 1
			add         eax, 2; update source ptr
			mov         dl, [eax]; red
			add         cl, [eax + edi]; green type 2
			rcr         cl, 1; real green
			add         ebx, 6; update dest ptr
			mov[ebx - 1], dl; write back red
			mov         dl, [eax + edi - 1]; blue
			mov[ebx - 2], cl; write back green
			mov[ebx - 3], dl; write back blue
			dec         esi
			jnz         Bayer3RGGBToRgb24Invert_odd_odd
			Bayer3RGGBToRgb24Invert_even_setup :
		mov         cl, [ebx - 3]
			mov         byte ptr[ebx], cl
			mov         cl, [ebx - 2]
			mov         byte ptr[ebx + 1], cl
			mov         cl, [ebx - 1]
			mov         byte ptr[ebx + 2], cl
			dec         height
			jz          Bayer3RGGBToRgb24Invert_done
			mov         esi, width; width - 1
			mov         ecx, edi; width
			shl         ecx, 2; width * 4
			add         ebx, 3
			add         ecx, edi; width * 5
			inc         eax; source ready
			add         ecx, edi; width * 6
			sub         ebx, ecx; dest ready
			Bayer3RGGBToRgb24Invert_even_odd :
		mov         cl, [eax]; green type 2
			mov         dl, [eax + edi]; red
			add         cl, [eax + edi + 1]; green type 1
			mov[ebx + 2], dl; write back red
			rcr         cl, 1; real green
			mov         dl, [eax + 1]; blue
			mov[ebx + 1], cl; write bac green
			dec         esi
			mov[ebx], dl; write back blue
			jnz         Bayer3RGGBToRgb24Invert_even_even
			inc         eax
			add         ebx, 3
			jmp         Bayer3RGGBToRgb24Invert_odd_setup
			Bayer3RGGBToRgb24Invert_even_even :
		mov         cl, [eax + 2]; green type 2
			add         ebx, 6
			add         cl, [eax + edi + 1]; green type 1
			mov         dl, [eax + edi + 2]; red
			rcr         cl, 1; real green
			add         eax, 2
			mov[ebx - 1], dl; write back red
			mov[ebx - 2], cl; write back green
			mov         dl, [eax - 1]; blue
			dec         esi
			mov[ebx - 3], dl; write back blue
			jnz         Bayer3RGGBToRgb24Invert_even_odd
			Bayer3RGGBToRgb24Invert_odd_setup :
		mov         cl, [ebx - 3]
			mov         byte ptr[ebx], cl
			mov         cl, [ebx - 2]
			mov         byte ptr[ebx + 1], cl
			mov         cl, [ebx - 1]
			mov         byte ptr[ebx + 2], cl
			dec         height
			jz          Bayer3RGGBToRgb24Invert_done
			add         ebx, 3
			mov         ecx, edi; width
			mov         esi, width; width - 1
			shl         ecx, 2; width * 4
			add         ecx, edi; width * 5
			add         ecx, edi; width * 6
			inc         eax; source ready
			sub         ebx, ecx; dest ready
			jmp         Bayer3RGGBToRgb24Invert_odd_odd
			Bayer3RGGBToRgb24Invert_done :
		xor         eax, eax; CvtSuccess
	}
	/* Replicate last line */
	memcpy(pDestSave + 3 * widthSave * (heightSave - 1),
		pDestSave + 3 * widthSave * (heightSave - 2),
		widthSave * 3);
	return TRUE;
}

int LaplacianFilter(BYTE *pSrc, DWORD width, DWORD height, BYTE *pDest)
{
	INT DeltH, DeltV, Temp;
	for (register DWORD i = 0; i<height; ++i) {//assume width and height all even number. don't make odd number
		BYTE *pLine = pSrc + i * width;
		BYTE *pRGBLine = pDest + (height - 1 - i)*width * 3;
		for (register DWORD j = 0; j<width; ++j) {
			BYTE *pNow = pLine + j;
			BYTE *pRGB = pRGBLine + j * 3;
			if (i == 0) {//First line
				if (!(j % 2))
				{ //it's R
					*pRGB = *(pNow + width + 1);//b
					*(pRGB + 1) = *(pNow + 1);
					*(pRGB + 2) = *pNow;
				}
				else {//g1
					*pRGB = *(pNow + width);
					*(pRGB + 1) = *pNow;
					*(pRGB + 2) = *(pNow - 1);
				}
				continue;
			}
			if (i == 1) {
				if (!(j % 2))
				{//it's g2
					*pRGB = *(pNow + 1);
					*(pRGB + 1) = *pNow;
					*(pRGB + 2) = *(pNow - width);
				}
				else {//it's b
					*pRGB = *pNow;
					*(pRGB + 1) = *(pNow - 1);
					*(pRGB + 2) = *(pNow - width - 1);
				}
				continue;
			}
			if (i == (height - 2)) {
				if (!(j % 2))
				{ //it's R
					*pRGB = *(pNow - width + 1);//b
					*(pRGB + 1) = *(pNow + 1);
					*(pRGB + 2) = *pNow;
				}
				else {
					*pRGB = *(pNow - width);
					*(pRGB + 1) = *pNow;
					*(pRGB + 2) = *(pNow - 1);
				}
				continue;
			}
			if (i == (height - 1)) {
				if (!(j % 2)) {//it's g2
					*pRGB = *(pNow + 1);
					*(pRGB + 1) = *pNow;
					*(pRGB + 2) = *(pNow - width);
				}
				else {//it's b
					*pRGB = *pNow;
					*(pRGB + 1) = *(pNow - 1);
					*(pRGB + 2) = *(pNow - width - 1);
				}
				continue;
			}
			if (j == 0) {
				if (!(i % 2)) {//r
					*pRGB = *(pNow + width + 1);//b
					*(pRGB + 1) = *(pNow + 1);
					*(pRGB + 2) = *pNow;
					continue;
				}
				else {//g2
					*pRGB = *(pNow + 1);
					*(pRGB + 1) = *pNow;
					*(pRGB + 2) = *(pNow - width);
					continue;
				}
			}
			if (j == 1) {
				if (!(i % 2)) {//g1
					*pRGB = *(pNow + width);
					*(pRGB + 1) = *pNow;
					*(pRGB + 2) = *(pNow - 1);
					continue;
				}
				else {//b
					*pRGB = *pNow;
					*(pRGB + 1) = *(pNow - 1);
					*(pRGB + 2) = *(pNow - width - 1);
					continue;
				}
			}
			/////////////////////////////////////////////////
			if (j == width - 2) {
				if (!(i % 2)) {//r
					*pRGB = *(pNow + width + 1);//b
					*(pRGB + 1) = *(pNow + 1);
					*(pRGB + 2) = *pNow;
					continue;
				}
				else {//g2
					*pRGB = *(pNow + 1);
					*(pRGB + 1) = *pNow;
					*(pRGB + 2) = *(pNow - width);
					continue;
				}
			}
			if (j == width - 1) {
				if (!(i % 2)) {//g1
					*pRGB = *(pNow + width);
					*(pRGB + 1) = *pNow;
					*(pRGB + 2) = *(pNow - 1);
					continue;
				}
				else {//b
					*pRGB = *pNow;
					*(pRGB + 1) = *(pNow - 1);
					*(pRGB + 2) = *(pNow - width - 1);
					continue;
				}
			}
			////////////////////////////////////////////////////
			if (!(i % 2) && !(j % 2)) {//it's R not calculate B
				DeltH = abs(*(pNow - 1) - *(pNow + 1)) + abs(2 * *pNow - *(pNow - 2) - *(pNow + 2));
				DeltV = abs(*(pNow - width) - *(pNow + width)) + abs(2 * *pNow - *(pNow - 2 * width) - *(pNow + 2 * width));
				*(pRGB + 2) = *(pNow);  //R
				if (DeltH<DeltV) {//calculate G
					Temp = (*(pNow - 1) + *(pNow + 1)) / 2 + (2 * *pNow - *(pNow - 2) - *(pNow + 2)) / 4;//G
					if (Temp>255) { Temp = 255; }
					if (Temp<0) Temp = 0;
					*(pRGB + 1) = (BYTE)Temp;
				}

				else if (DeltH>DeltV) {
					Temp = (*(pNow - width) + *(pNow + width)) / 2 + (2 * *pNow - *(pNow - 2 * width) - *(pNow + 2 * width)) / 4;
					if (Temp > 255) Temp = 255;
					if (Temp < 0) Temp = 0;
					*(pRGB + 1) = (BYTE)Temp;
				}
				else {
					Temp = (*(pNow - 1) + *(pNow + 1) + *(pNow + width) + *(pNow - width)) / 4 + (4 * (*pNow) - *(pNow - 2) - *(pNow + 2) - *(pNow + 2 * width) - *(pNow - 2 * width)) / 8;
					if (Temp > 255) Temp = 255;
					if (Temp < 0) Temp = 0;
					*(pRGB + 1) = (BYTE)Temp;
				}
			}
			else if ((i + j) % 2) {//It's G OddG and EvenG not calculate R,B
				*(pRGB + 1) = *pNow;
			}
			else {//It's B not calculate R
				DeltH = abs(*(pNow - 1) - *(pNow + 1)) + abs(2 * *pNow - *(pNow - 2) - *(pNow + 2));
				DeltV = abs(*(pNow - width) - *(pNow + width)) + abs(2 * *pNow - *(pNow - 2 * width) - *(pNow + 2 * width));
				*(pRGB) = *(pNow);  //B
				if (DeltH<DeltV) {//calculate G
					Temp = (*(pNow - 1) + *(pNow + 1)) / 2 + (2 * *pNow - *(pNow - 2) - *(pNow + 2)) / 4;//G
					if (Temp > 255) Temp = 255;
					if (Temp < 0) Temp = 0;
					*(pRGB + 1) = (BYTE)Temp;
				}
				else if (DeltH>DeltV) {
					Temp = (*(pNow - width) + *(pNow + width)) / 2 + (2 * *pNow - *(pNow - 2 * width) - *(pNow + 2 * width)) / 4;
					if (Temp > 255) Temp = 255;
					if (Temp < 0) Temp = 0;
					*(pRGB + 1) = (BYTE)Temp;
				}
				else {
					Temp = (*(pNow - 1) + *(pNow + 1) + *(pNow + width) + *(pNow - width)) / 4 + (4 * (*pNow) - *(pNow - 2) - *(pNow + 2) - *(pNow + 2 * width) - *(pNow - 2 * width)) / 8;
					if (Temp > 255) Temp = 255;
					if (Temp < 0) Temp = 0;
					*(pRGB + 1) = (BYTE)Temp;
				}
			}
		}
	}
	//////////////////////////////all G has fill, some R and B isn't fill///////////////////////////
	INT DeltN, DeltP;
	int i = 0;
	for (i = 2; i<height - 2; ++i) {
		BYTE *pLine = pSrc + i * width;
		BYTE *pRGBLine = pDest + (height - 1 - i)*width * 3;
		for (register DWORD j = 2; j<width - 2; ++j) {
			BYTE *pNow = pLine + j;
			BYTE *pRGB = pRGBLine + j * 3;
			if (!(i % 2) && !(j % 2)) {//it's R calculate B
				DeltN = abs(*(pNow - width - 1) - *(pNow + width + 1)) + abs(2 * *(pRGB + 1) - *(pRGB + 1 + 3 * width - 3) - *(pRGB + 1 - 3 * width + 3));
				DeltP = abs(*(pNow - width + 1) - *(pNow + width - 1)) + abs(2 * *(pRGB + 1) - *(pRGB + 1 + 3 * width + 3) - *(pRGB + 1 - 3 * width - 3));
				if (DeltN<DeltP) {
					Temp = (*(pNow - width - 1) + *(pNow + width + 1)) / 2 + (2 * *(pRGB + 1) - *(pRGB + 1 + 3 * width - 3) - *(pRGB + 1 - 3 * width + 3)) / 2;//G
					if (Temp>255) Temp = 255;
					if (Temp<0) Temp = 0;
					*(pRGB) = (BYTE)Temp;
				}
				else if (DeltN>DeltP) {
					Temp = (*(pNow - width + 1) + *(pNow + width - 1)) / 2 + (2 * *(pRGB + 1) - *(pRGB + 1 + 3 * width + 3) - *(pRGB + 1 - 3 * width - 3)) / 2;
					if (Temp>255) Temp = 255;
					if (Temp<0) Temp = 0;
					*(pRGB) = (BYTE)Temp;
				}
				else {
					Temp = (*(pNow - width - 1) + *(pNow + width + 1) + *(pNow - width + 1) + *(pNow + width - 1)) / 4 + (4 * *(pRGB + 1) - *(pRGB + 1 + 3 * width - 3) - *(pRGB + 1 - 3 * width + 3) - *(pRGB + 1 + 3 * width + 3) - *(pRGB + 1 - 3 * width - 3)) / 4;
					if (Temp>255) Temp = 255;
					if (Temp<0) Temp = 0;
					*(pRGB) = (BYTE)Temp;
				}
			}
			else if ((i + j) % 2) {//It's G OddG and EvenG not calculate R,B
				if (j % 2) {//Odd G
					Temp = (*(pNow - width) + *(pNow + width)) / 2 + (2 * *(pRGB + 1) - *(pRGB + 1 - 3 * width) - *(pRGB + 1 + 3 * width)) / 4; //B
					if (Temp>255) Temp = 255;
					if (Temp<0) Temp = 0;
					*(pRGB) = (BYTE)Temp;
					Temp = (*(pNow - 1) + *(pNow + 1)) / 2 + (2 * *(pRGB + 1) - *(pRGB + 1 + 3) - *(pRGB + 1 - 3)) / 4;				//R
					if (Temp>255) Temp = 255;
					if (Temp<0) Temp = 0;
					*(pRGB + 2) = (BYTE)Temp;
				}
				else { //Even G
					Temp = (*(pNow - width) + *(pNow + width)) / 2 + (2 * *(pRGB + 1) - *(pRGB + 1 - 3 * width) - *(pRGB + 1 + 3 * width)) / 4; //r
					if (Temp>255) Temp = 255;
					if (Temp<0) Temp = 0;
					*(pRGB + 2) = (BYTE)Temp;
					Temp = (*(pNow - 1) + *(pNow + 1)) / 2 + (2 * *(pRGB + 1) - *(pRGB + 1 + 3) - *(pRGB + 1 - 3)) / 4;				//b
					if (Temp>255) Temp = 255;
					if (Temp<0) Temp = 0;
					*(pRGB) = (BYTE)Temp;
				}
			}
			else {//It's B  calculate R
				DeltN = abs(*(pNow - width - 1) - *(pNow + width + 1)) + abs(2 * *(pRGB + 1) - *(pRGB + 1 + 3 * width - 3) - *(pRGB + 1 - 3 * width + 3));
				DeltP = abs(*(pNow - width + 1) - *(pNow + width - 1)) + abs(2 * *(pRGB + 1) - *(pRGB + 1 + 3 * width + 3) - *(pRGB + 1 - 3 * width - 3));
				if (DeltN<DeltP) {
					Temp = (*(pNow - width - 1) + *(pNow + width + 1)) / 2 + (2 * *(pRGB + 1) - *(pRGB + 1 + 3 * width - 3) - *(pRGB + 1 - 3 * width + 3)) / 2;//G
					if (Temp>255) Temp = 255;
					if (Temp<0) Temp = 0;
					*(pRGB + 2) = (BYTE)Temp;
				}
				else if (DeltN>DeltP) {
					Temp = (*(pNow - width + 1) + *(pNow + width - 1)) / 2 + (2 * *(pRGB + 1) - *(pRGB + 1 + 3 * width + 3) - *(pRGB + 1 - 3 * width - 3)) / 2;
					if (Temp>255) Temp = 255;
					if (Temp<0)   Temp = 0;
					*(pRGB + 2) = (BYTE)Temp;
				}
				else {
					Temp = (*(pNow - width - 1) + *(pNow + width + 1) + *(pNow - width + 1) + *(pNow + width - 1)) / 4 + (4 * *(pRGB + 1) - *(pRGB + 1 + 3 * width - 3) - *(pRGB + 1 - 3 * width + 3) - *(pRGB + 1 + 3 * width + 3) - *(pRGB + 1 - 3 * width - 3)) / 4;
					if (Temp>255) Temp = 255;
					if (Temp<0) Temp = 0;
					*(pRGB + 2) = (BYTE)Temp;
				}
			}
		}
	}

	return 0;
}


// 读取位图函数
unsigned char* readBmp(char* bmpName)
{
	FILE* fp = fopen(bmpName, "rb"); //以二进制读的方式打开指定的图像文件
	if (fp == 0) return 0;

	fseek(fp, sizeof(BITMAPFILEHEADER), 0);
	BITMAPINFOHEADER infoHead;
	fread(&infoHead, sizeof(BITMAPINFOHEADER), 1, fp);

	bmpWidth = infoHead.biWidth;
	bmpHeight = infoHead.biHeight;
	biBitCount = infoHead.biBitCount;

	//strick
	int lineByte = (bmpWidth * biBitCount / 8 + 3) / 4 * 4;
	strick = lineByte;
	if (biBitCount == 8)
	{
		pColorTable = new RGBQUAD[256];
		fread(pColorTable, sizeof(RGBQUAD), 256, fp);

	}
	pBmpBuf = new unsigned char[lineByte * bmpHeight];

	fread(pBmpBuf, 1, lineByte * bmpHeight, fp);
	fclose(fp);
	return pBmpBuf;
}
// _CRT_SECURE_NO_WARNINGS


void noise_do(Mat image, int n)
{
	int i, j;
	for (int k = 0; k<n; k++)
	{
		// rand()是随机数生成器  
		i = rand() % image.cols;
		j = rand() % image.rows;
		if (image.type() == CV_8UC1)
		{ // 灰度图像  
			image.at<uchar>(j, i) = 255;
		}
		else if (image.type() == CV_8UC3)
		{ // 彩色图像  
			image.at<cv::Vec3b>(j, i)[0] = 255;
			image.at<cv::Vec3b>(j, i)[1] = 255;
			image.at<cv::Vec3b>(j, i)[2] = 255;
		}
	}
}


//主函数
int main()
{
	/*************************************** 第一步 *********************************************/
	unsigned char* uchar_input = readBmp("inputbmp.bmp"); // unsigned char* 格式

	//Mat(int rows, int cols, int type, void* data, size_t step = AUTO_STEP);
	Mat Mat_input(bmpHeight, bmpWidth, CV_8UC3, uchar_input, strick);  // 将图像转换为Mat格式的

	Mat INPUT_IMAGE;
	flip(Mat_input, INPUT_IMAGE, 0);// 水平方向旋转180度



	cv::imshow("原图像", INPUT_IMAGE);//第一步：显示源图像
	waitKey();
	/*************************************** 第二步 *********************************************/
	// 第二步：分别提取图像 B G R三个通道
	for (int i = 0; i < 3; i++)
	{
		Mat bgr(INPUT_IMAGE.rows, INPUT_IMAGE.cols, CV_8UC3, Scalar(0, 0, 0));
		Mat temp(INPUT_IMAGE.rows, INPUT_IMAGE.cols, CV_8UC1);
		Mat out[] = { bgr };
		int from_to[] = { i,i };
		mixChannels(&INPUT_IMAGE, 1, out, 1, from_to, 1);  // 单个通道的提取
		//获得其中一个通道的数据进行分析  
		if(i == 0) imshow("单通道图像（蓝色通道）", bgr);
		if (i == 1) imshow("单通道图像（绿色通道）", bgr);
		if (i == 2) imshow("单通道图像（红色通道）", bgr);
		waitKey();
	}
	/*************************************** 第三步 *********************************************/
	// 显示1.5倍亮度 图像亮度变换  
	Mat LIANGDU_IMAHE = INPUT_IMAGE.clone();


	int height = INPUT_IMAGE.rows;
	int width = INPUT_IMAGE.cols;
	Mat dst_mat = Mat::zeros(INPUT_IMAGE.size(), INPUT_IMAGE.type());
	float alpha = 1.8;
	float beta = 20;

	//Mat m1;  
	//src.convertTo(m1,CV_32F);  
	for (int row = 0; row < height; row++)
	{
		for (int col = 0; col < width; col++)
		{
			if (INPUT_IMAGE.channels() == 3)
			{
				float b = INPUT_IMAGE.at<Vec3b>(row, col)[0];//blue  
				float g = INPUT_IMAGE.at<Vec3b>(row, col)[1];//green  
				float r = INPUT_IMAGE.at<Vec3b>(row, col)[2];//red  

														 //output  
				dst_mat.at<Vec3b>(row, col)[0] = saturate_cast<uchar>(b*alpha + beta);
				dst_mat.at<Vec3b>(row, col)[1] = saturate_cast<uchar>(g*alpha + beta);
				dst_mat.at<Vec3b>(row, col)[2] = saturate_cast<uchar>(r*alpha + beta);
			}
			else if (INPUT_IMAGE.channels() == 1)
			{
				float gray = INPUT_IMAGE.at<uchar>(row, col);
				dst_mat.at<uchar>(row, col) = saturate_cast<uchar>(gray*alpha + beta);
			}
		}
	}
	imshow("1.5倍亮度", dst_mat);
	waitKey();
	/*************************************** 第四步 *********************************************/
	cv::Mat srcGray;//创建无初始化矩阵

	cvtColor(INPUT_IMAGE, srcGray, CV_RGB2GRAY);
	imshow("灰度图像", srcGray);
	waitKey();
	/*************************************** 第五步 *********************************************/
	// 灰度图像的有序抖动
	BYTE BayerPattern[8][8] = { 
		0, 32,  8, 40,  2, 34, 10, 42,
		48, 16, 56, 24, 50, 18, 58, 26,
		12, 44,  4, 36, 14, 46,  6, 38,
		60, 28, 52, 20, 62, 30, 54, 22,
		3, 35, 11, 43,  1, 33,  9, 41,
		51, 19, 59, 27, 49, 17, 57, 25,
		15, 47,  7, 39, 13, 45,  5, 37,
		63, 31, 55, 23, 61, 29, 53, 21 };

	unsigned char* uchar_output;
	uchar_output = uchar_input;
	noise_do(srcGray,50000);
	imshow("有序抖动1", srcGray);
	waitKey();
	BayerFilter(srcGray.data, bmpWidth, bmpHeight, uchar_output);
	Mat Mat_input_Bayer(bmpHeight, bmpWidth, CV_8UC3, uchar_output, strick);  // 将图像转换为Mat格式的
	flip(Mat_input_Bayer, Mat_input_Bayer, 0);// 水平方向旋转180度
	imshow("有序抖动2", Mat_input_Bayer);
	waitKey();
	return 0;
}
