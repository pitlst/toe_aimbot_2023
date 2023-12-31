/*
    Before inferencing, image preprocess
*/
#include <iostream>
#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <cuda_runtime.h>
__global__ void linear(const uchar* srcData, const int srcH, const int srcW, uchar* tgtData, const int tgtH, const int tgtW)
{
    int ix = threadIdx.x + blockIdx.x * blockDim.x;
    int iy = threadIdx.y + blockIdx.y * blockDim.y;
    int idx = ix + iy * tgtW;
    int idx3 = idx * 3;

    float scaleY = (float)tgtH / (float)srcH;
    float scaleX = (float)tgtW / (float)srcW;

    // (ix,iy)为目标图像坐标
    // (before_x,before_y)原图坐标
    float beforeX = float(ix + 0.5) / scaleX - 0.5;
    float beforeY = float(iy + 0.5) / scaleY - 0.5;
    // 原图像坐标四个相邻点
    // 获得变换前最近的四个顶点,取整
    int topY = static_cast<int>(beforeY);
    int bottomY = topY + 1;
    int leftX = static_cast<int>(beforeX);
    int rightX = leftX + 1;
    //计算变换前坐标的小数部分
    float u = beforeX - leftX;
    float v = beforeY - topY;

    if (ix < tgtW && iy < tgtH)
    {
        // 如果计算的原始图像的像素大于真实原始图像尺寸
        if (topY >= srcH - 1 && leftX >= srcW - 1)  //右下角
        {
            for (int k = 0; k < 3; k++)
            {
                tgtData[idx3 + k] = (1. - u) * (1. - v) * srcData[(leftX + topY * srcW) * 3 + k];
            }
        }
        else if (topY >= srcH - 1)  // 最后一行
        {
            for (int k = 0; k < 3; k++)
            {
                tgtData[idx3 + k]
                = (1. - u) * (1. - v) * srcData[(leftX + topY * srcW) * 3 + k]
                + (u) * (1. - v) * srcData[(rightX + topY * srcW) * 3 + k];
            }
        }
        else if (leftX >= srcW - 1)  // 最后一列
        {
            for (int k = 0; k < 3; k++)
            {
                tgtData[idx3 + k]
                = (1. - u) * (1. - v) * srcData[(leftX + topY * srcW) * 3 + k]
                + (1. - u) * (v) * srcData[(leftX + bottomY * srcW) * 3 + k];
            }
        }
        else  // 非最后一行或最后一列情况
        {
            for (int k = 0; k < 3; k++)
            {
                tgtData[idx3 + k]
                = (1. - u) * (1. - v) * srcData[(leftX + topY * srcW) * 3 + k]
                + (u) * (1. - v) * srcData[(rightX + topY * srcW) * 3 + k]
                + (1. - u) * (v) * srcData[(leftX + bottomY * srcW) * 3 + k]
                + u * v * srcData[(rightX + bottomY * srcW) * 3 + k];
            }
        }
    }
}



__global__ void process(const uchar* srcData, float* tgtData, const int h, const int w)
{

    int ix = threadIdx.x + blockIdx.x * blockDim.x;
    int iy = threadIdx.y + blockIdx.y * blockDim.y;
    int chw = h* w;
    if (ix < w && iy < h)
    {
        int idx = ix + iy * w;
        int idx3 = idx * 3;
        tgtData[idx] = static_cast<float>(srcData[idx3]) / 255.0;// R pixel
        tgtData[idx + chw] = static_cast<float>(srcData[idx3 + 1]) / 255.0; // G pixel
        tgtData[idx + chw* 2] = static_cast<float>(srcData[idx3 + 2]) / 255.0;  // B pixel    
    }
}



void ppreprocess(const cv::Mat& srcImg, void* dstData, const int dstHeight, const int dstWidth,
                    float* dstDevData, uchar* midDevData, uchar*srcDevData,
                    const int srcHeight, const int srcWidth)
{
    int srcElements = srcHeight * srcWidth * 3;
    int dstElements = dstHeight * dstWidth * 3;
    
    cudaMemcpy(srcDevData, srcImg.data, sizeof(uchar) * srcElements, cudaMemcpyHostToDevice);

    dim3 blockSize(32,32);
    dim3 gridSize((dstWidth + blockSize.x - 1) / blockSize.x, (dstHeight + blockSize.y - 1) / blockSize.y);

    //printf("Block(%d, %d),Grid(%d, %d).\n", blockSize.x, blockSize.y, gridSize.x, gridSize.y);

    // resize
    // linear<<<gridSize, blockSize>>>(srcDevData, srcHeight, srcWidth, midDevData, dstHeight, dstWidth);
    // hwc to chw / bgr to rgb / normalize
    // cudaDeviceSynchronize();
    float* fuck = static_cast<float*>(dstData);
    process<<<gridSize, blockSize>>>(srcDevData, fuck, dstHeight, dstWidth);
    //process<<<gridSize, blockSize>>>(midDevData, dstDevData, dstHeight, dstWidth);
    //cudaMemcpy(dstData, dstDevData, sizeof(float) * dstElements, cudaMemcpyDeviceToDevice);
    //cudaMemcpy(out, dstDevData, sizeof(float) * dstElements, cudaMemcpyDeviceToHost);

}


