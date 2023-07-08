#ifndef ORIN_DETECT_H_
#define ORIN_DETECT_H_

#ifdef USE_NVIDIA

#include <vector>
#include <memory>

#include <opencv2/opencv.hpp>

#include "cuda_expand.cuh"

#include "detect.hpp"
#include "general.hpp"

#include "NvInfer.h"
#include "NvOnnxParser.h"
#include "logger.h"
#include "common.h"
#include "buffers.h"

// 注意：对于网络部分输入输出层的字节数进行硬编码，在nx推理程序中未注释的数字基本均为此原因
#define OUTPUT_SIZE_8 1632000
#define OUTPUT_SIZE_16 408000
#define OUTPUT_SIZE_32 102000
#define OUTPUT_SIZE_G640 2142000

namespace toe
{   
    class Detector final : public Detector_base 
    {
    public:
        Detector() = default;
        ~Detector() = default;
        void Init(const toe::json_head & input_json, int color);

        void* buffers_[5];

    private:
        void preprocess() override;
        void inference() override;
        void postprocess() override;

    public:
        // TensorRT推理相关
        std::unique_ptr<nvinfer1::IRuntime> runtime_;
        std::unique_ptr<nvinfer1::IExecutionContext> context_;
        cudaStream_t stream_;
        // 单次推理的图片数, 对于nx强制为2
        const int batch_size_ = 2;
        // 缓存的指针数
        void* buffers_[5];
        // 指定张量的索引
        int inputIndex;
        int output_stride8;
        int output_stride16;
        int output_stride32;
        // 输入的图像
        cv::Mat pr_img;
        cv::Mat show_left;
        cv::Mat show_right;
    private:

        // 数据处理相关的指针
        float* dstDevData;
        float* dstDevData_right;
        uchar* midDevData;
        uchar* midDevData_right;
        uchar* srcDevData_left;
        uchar* srcDevData_right;

        float* outputs_decode;
        float* outputs_decode_right;
        int* outputs_decode_nums;
        int* outputs_decode_nums_right;
        float* final_out;
        float* final_out_right;

        static float data[1* 3* 640*640];
        static float data2[1* 3* 640*640];
    };
}

#endif
#endif