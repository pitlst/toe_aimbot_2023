#include <iostream>
#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <cuda_runtime.h>
#include "common_structs.h"
#include <thrust/sort.h>
#include <thrust/device_ptr.h>

using namespace cv;
using namespace std;

__device__ float calc_iou(float* a, float *b)
{
    // 获取矩形框a的坐标
    float x0_a = a[0];
    float y0_a = a[1];
    float x1_a = a[2];
    float y1_a = a[3];
    
    // 获取矩形框b的坐标
    float x0_b = b[0];
    float y0_b = b[1];
    float x1_b = b[2];
    float y1_b = b[3];
    
    // 计算交集面积
    float intersectionArea = max(0.0f, min(x1_a, x1_b) - max(x0_a, x0_b)) * max(0.0f, min(y1_a, y1_b) - max(y0_a, y0_b));
    
    // 计算并集面积
    float area_a = (x1_a - x0_a) * (y1_a - y0_a);
    float area_b = (x1_b - x0_b) * (y1_b - y0_b);
    float unionArea = area_a + area_b - intersectionArea;
    
    // 计算iou值
    float iou = intersectionArea / unionArea;
    
    return iou;
}

__device__ float sigmoid(float x)
{
    return 1.0 / (1.0 + expf(-x));
}


__device__ int argmax(const float* data, int size)
{
    int max_idx = 0;
    float max_val = data[0];
    for (int i = 1; i < size; ++i)
    {
        if (data[i] > max_val)
        {
            max_val = data[i];
            max_idx = i;
        }
    }
    return max_idx;
}


// max_size x (x0, y0, x1, y1, x2, y2, x3, y3, x4, y4, 
// t_size, type, conf, color, x0, y0, x1, y1)

__global__ void merge_nms_kernel(float* decode, float* out, int nums_decode, int* nums_out, 
                        float nms_thresh, float merge_thresh)//, int* thresh, int* out_idx)
{
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid == 0)
    {
        //atomicAdd(nums_out, 1);
        int idx[200];
        int picked[20];
        
        for (int i=0; i<nums_decode; i++) 
        {
            idx[i] = i;
        }
        
        for (int i=0; i<nums_decode-1;i++)
            for (int j=0; j<nums_decode-i-1;j++)
                if (decode[idx[j]*18+12] < decode[idx[j+1]*18+12])
                {
                    int temp = idx[j];
                    idx[j] = idx[j+1];
                    idx[j+1] = temp;
                }
        //for (int i=0; i<nums_decode; i++)
        //{
        //    thresh[i] = int(100*decode[idx[i]*18+12]);
        //    out_idx[i] = idx[i];
        //}
        int picked_size = 0;
        //*nums_out = idx[0];
        for (int i=0; i<nums_decode; i++)
        {
            int idxi = idx[i];
            if (decode[idxi*18+12] == 0) continue;
            bool keep = 1;
            //if(idxi==6) *nums_out += 1;
            for (int j=0; j<picked_size; j++)
            {
                int idxj = picked[j];
                float iou = calc_iou(decode+idxi*18+14, decode+idxj*18+14);
                
                if (iou > nms_thresh || isnan(iou))
                {
                    keep = 0;
                    break;
                }
            }
            if (keep)
            {
                picked[picked_size] = idxi;
                picked_size++;
            }
        }

        for (int i=0;i<picked_size;i++)
        {
            int idx_out = idx[picked[i]];
            for (int j=0;j<18;j++)
                out[i*18+j] = decode[idx_out*18+j];
        }
        *nums_out = picked_size;
    }
}

__global__ void decode_outputs_kernel(float *prob, float* l_anchor,
                                      const int stride, const int num_out, const int out_h, const int out_w,
                                      const int img_w, const int img_h, const float bbox_conf_thresh,
                                      const int classes, const int colors, const int camp,
                                      float* objects, float scale, int* nums)
{
    int na = blockIdx.z * blockDim.z + threadIdx.z;
    int h_id = blockIdx.y * blockDim.y + threadIdx.y;
    int w_id = blockIdx.x * blockDim.x + threadIdx.x;
    //*objects = -999;
    //*nums = int(prob[1999]*10000);
    if (na < 3 && h_id < out_h && w_id < out_w)
    {
        // int data_idx = (na * out_h * out_w + h_id * out_w + w_id) * num_out;
        int data_idx = (na * out_h * out_w + h_id * out_w + w_id) * num_out;
        //*nums = na;
        float obj_conf = sigmoid(prob[data_idx + 4]);
        //float obj_conf = prob[data_idx + 4];
        //prob[data_idx] = 100;
        if (obj_conf > bbox_conf_thresh)
        {
            float *color = &prob[data_idx + 15 + classes];
            float *cls = &prob[data_idx + 15];
            float *size = &prob[data_idx + 15 + classes + colors];
            int col_id = argmax(color, colors);
            int cls_id = argmax(cls, classes);

            int t_size = argmax(size, 2);
            
            double final_conf =
                obj_conf * pow(sigmoid(prob[data_idx +15 + cls_id]) *
                                   sigmoid(prob[data_idx +15 + classes + col_id]) *
                                   sigmoid(prob[data_idx +15 + classes + colors + t_size]),
                               1 / 3.);
            if (final_conf > bbox_conf_thresh)
            {
                int n = atomicAdd(nums, 1)-1;
                float x = (sigmoid(prob[data_idx]) * 2.0 - 0.5 + w_id) * stride;
                float y = (sigmoid(prob[data_idx+1]) * 2.0 - 0.5 + h_id) * stride;
                float w = (sigmoid(prob[data_idx+2]) * 2)*(sigmoid(prob[data_idx+2]) * 2) * l_anchor[na * 2];
                float h = (sigmoid(prob[data_idx+3]) * 2)*(sigmoid(prob[data_idx+3]) * 2) * l_anchor[na * 2 + 1];
                for (int p = 0; p < 5; ++p)
                {
                    float px = (prob[data_idx+5 + p * 2] * l_anchor[na * 2] + w_id * stride) / scale;
                    float py = (prob[data_idx+5 + p * 2 + 1] * l_anchor[na * 2 + 1] + h_id * stride) / scale;
                    px = max(min(px, (float)(img_w)), 0.f);
                    py = max(min(py, (float)(img_h)), 0.f);
                    objects[n*18+p*2] = px;
                    objects[n*18+p*2+1] = py;
                }
                float x0 = (x - w * 0.5) / scale;
                float y0 = (y - h * 0.5) / scale;
                float x1 = (x + w * 0.5) / scale;
                float y1 = (y + h * 0.5) / scale;

                x0 = max(min(x0, (float)(img_w)), 0.f);
                y0 = max(min(y0, (float)(img_h)), 0.f);
                x1 = max(min(x1, (float)(img_w)), 0.f);
                y1 = max(min(y1, (float)(img_h)), 0.f);

                objects[n*18+10] = t_size;
                objects[n*18+11] = cls_id;
                objects[n*18+12] = final_conf;
                objects[n*18+13] = col_id;
                objects[n*18+14] = x0;
                objects[n*18+15] = y0;
                objects[n*18+16] = x1;
                objects[n*18+17] = y1;
            }
        
        }
    }
    
}

void decode_outputs_cu(float *prob, float* objects,
                    int stride, int num_out, const int img_w,
                    const int img_h, s_detector_params& param_, int* nums)
{
    float scale = min(param_.w / (img_w * 1.0), param_.h / (img_h * 1.0));
    //cout << "scale:  " << scale << endl;
    float anchor[6];
    float* l_anchor;
    switch (stride)
    {
    case 4:
        for (int k=0;k<6;k++)
            anchor[k] = param_.a1[k];
        break;
    case 8:
        for (int k=0;k<6;k++)
            anchor[k] = param_.a2[k];
        break;
    case 16:
        for (int k=0;k<6;k++)
            anchor[k] = param_.a3[k];
        break;
    case 32:
        for (int k=0;k<6;k++)
            anchor[k] = param_.a4[k];
        break;
    default:
        assert(false && "Unknown layer stride!");
        break;
    }

    int out_h = param_.h / stride;
    int out_w = param_.w / stride;

    dim3 blockSize(32, 32);
    dim3 gridSize((out_h + blockSize.x - 1) / blockSize.x, (out_w + blockSize.y - 1) / blockSize.y, 3);

    cudaMalloc(&l_anchor, sizeof(float) * 6);
    cudaMemcpy(l_anchor, anchor, sizeof(float)*6, cudaMemcpyHostToDevice);

    decode_outputs_kernel<<<gridSize, blockSize>>>(prob, l_anchor,
                                                   stride, num_out, out_h, out_w,
                                                   img_w, img_h, param_.bbox_conf_thresh,
                                                   param_.classes, param_.colors, param_.camp,
                                                   objects, scale, nums);
    cudaDeviceSynchronize();
}
void do_nms(float* objects, float* final_out, int* nums, s_detector_params& param_, int n)
{
    dim3 blockSize(1);
    dim3 gridSize(1);
    //int idx_out[n];
    //int* idx_out_ptr;
    //udaMalloc((void**)&idx_out_ptr, sizeof(int)*n);
    //int thresh_out[n];
    //int* thresh_out_ptr;
    //cudaMalloc((void**)&thresh_out_ptr, sizeof(int)*n);
    
    //cudaDeviceSynchronize();
    merge_nms_kernel<<<gridSize, blockSize>>>(objects, final_out, 
                                n, nums, 
                            param_.nms_thresh, param_.merge_thresh);//,
                            //idx_out_ptr, thresh_out_ptr);
    //cudaMemcpy(idx_out, idx_out_ptr, sizeof(int)*n, cudaMemcpyDeviceToHost);
    //cudaMemcpy(thresh_out, thresh_out_ptr, sizeof(int)*n, cudaMemcpyDeviceToHost);
    //for (int i=0;i<n;i++)
    //{
    //    cout << thresh_out[i] << " ";
    //}
    //cout << endl;
    //for (int i=0;i<n;i++)
    //{
    //    cout << idx_out[i] << " ";
    //}
    cudaDeviceSynchronize();

}
