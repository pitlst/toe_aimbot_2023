#ifdef USE_NVIDIA

#include "orin_detect.hpp"

void toe::Detector::Init(const toe::json_head & input_json, int color)
{
    // 父类初始化 
    toe::Detector_base::Init(input_json, color);
    // 子类初始化
    std::cout << "load engine file path is " << param_.engine_file_path << std::endl;

    std::vector<unsigned char> engine_data_;
    std::shared_ptr<nvinfer1::ICudaEngine> engine_;
    int length;

    // 读取engine文件
    std::ifstream engine_file(param_.engine_file_path, std::ios::binary);
    assert(engine_file.is_open() && "Unable to load engine_ file.");
    // 获取文件字节数
    engine_file.seekg(0, engine_file.end);
    length = engine_file.tellg();
    engine_file.seekg(0, engine_file.beg);
    // 转换读取内容
    engine_file.read(reinterpret_cast<char *>(engine_data_.data()), length);
    // 初始化推理框架
    runtime_ = std::unique_ptr<nvinfer1::IRuntime>(nvinfer1::createInferRuntime(sample::gLogger.getTRTLogger()));
    if (!runtime_)
    {
        throw std::runtime_error("runtime_ create failed");
    }
    engine_ = std::shared_ptr<nvinfer1::ICudaEngine>(runtime_->deserializeCudaEngine(engine_data_.data(), engine_data_.size()));
    if (!engine_)
    {
        throw std::runtime_error("engine_ init failed");
    }
    context_ = std::unique_ptr<nvinfer1::IExecutionContext>(engine_->createExecutionContext());
    if (!context_)
    {
        throw std::runtime_error("context_ create failed");
    }

    inputIndex = engine_->getBindingIndex("data");
    output_stride8 = engine_->getBindingIndex("stride_8");
    output_stride16 = engine_->getBindingIndex("stride_16");
    output_stride32 = engine_->getBindingIndex("stride_32");
    
    // 申请内存
    cudaMalloc((void**)&dstDevData, sizeof(float) * 640*640*3);
    cudaMalloc((void**)&dstDevData_right, sizeof(float) * 640*640*3);
    cudaMalloc((void**)&midDevData, sizeof(uchar) * 640*640*3);
    cudaMalloc((void**)&midDevData_right, sizeof(uchar) * 640*640*3);
    cudaMalloc((void**)&srcDevData_left, sizeof(uchar) * 640*640*3);
    cudaMalloc((void**)&srcDevData_right, sizeof(uchar) * 640*640*3);
    // max_size x (x0, y0, x1, y1, x2, y2, x3, y3, x4, y4, t_size, type, conf, color,x0,y0,x1,y1)
    cudaMalloc((void**)&outputs_decode, 200*  18* sizeof(float));
    cudaMalloc((void**)&outputs_decode_right, 200*  18* sizeof(float));
    // nums 1 int
    cudaMalloc((void**)&outputs_decode_nums, sizeof(int));
    cudaMalloc((void**)&outputs_decode_nums_right, sizeof(int));
    //final out
    cudaMalloc((void**)&final_out,20*  18* sizeof(float));
    cudaMalloc((void**)&final_out_right,20*  18* sizeof(float));
    // Create GPU buffers_ on device
    CHECK(cudaMallocHost(&buffers_[inputIndex], batch_size_ * 3 * param_.h * param_.w * sizeof(float)));
    CHECK(cudaMallocHost(&buffers_[output_stride8], batch_size_ * 3*80*80*29   * sizeof(float)));
    CHECK(cudaMallocHost(&buffers_[output_stride16], batch_size_ * 3*40*40*29   * sizeof(float)));
    CHECK(cudaMallocHost(&buffers_[output_stride32], batch_size_ * 3*20*20*29   * sizeof(float)));
    
    CHECK(cudaStreamCreate(&stream_));
}

void toe::Detector::preprocess()
{
    ppreprocess(input_img2_, buffers_[inputIndex]
        , 640, 640, dstDevData, midDevData, srcDevData_left, 640, 640);
    cudaDeviceSynchronize();
    ppreprocess(input_img, buffers_[inputIndex]+640*640*3*sizeof(float)
        , 640, 640, dstDevData_right, midDevData_right, srcDevData_right, 640, 640);
    cudaDeviceSynchronize();
}

void toe::Detector::inference()
{
    auto start = std::chrono::high_resolution_clock::now();
    //context_->setBindingDimensions(0, Dims4(2,3,640,640));
    context_->executeV2(buffers_);
    //context_->executeV2(buffers_);
    //context_->enqueueV2(buffers_, stream_, nullptr);
    cudaStreamSynchronize(stream_);
    //cudaDeviceSynchronize();
    auto end = std::chrono::high_resolution_clock::now();
    float elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    //cudaStreamSynchronize(stream_);
    //cout << "infer: " << elapsed/1000 << endl;
}

void toe::Detector::postprocess()
{
    
}

#endif