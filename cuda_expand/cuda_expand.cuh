#ifndef CUDA_EXPAND_H_
#define CUDA_EXPAND_H_

#include <iostream>
#include <stdio.h>
#include <opencv2/opencv.hpp>

#include "toe_structs.hpp"
#include "cuda_runtime.h"

extern __global__ void linear(const uchar* srcData, const int srcH, const int srcW, uchar* tgtData, const int tgtH, const int tgtW);

extern __global__ void process(const uchar* srcData, float* tgtData, const int h, const int w);

extern void ppreprocess(const cv::Mat& srcImg, void* dstData, const int dstHeight, const int dstWidth,
                    float* dstDevData, uchar* midDevData, uchar*srcDevData,
                    const int srcHeight, const int srcWidth);
extern void decode_outputs_cu(float *prob, float* objects,
                    int stride, int num_out, const int img_w,
                    const int img_h, detect_data& param_, int* nums);
extern void do_nms(float* objects, float* final_out, int* nums, detect_data& param_, int n);


#endif