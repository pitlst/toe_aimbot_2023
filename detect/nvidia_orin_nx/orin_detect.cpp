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

    std::cout << inputIndex << std::endl;
    std::cout << output_stride8 << std::endl;
    std::cout << output_stride16 << std::endl;
    std::cout << output_stride32 << std::endl;

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
    CHECK(cudaMallocHost(&buffers_[inputIndex], param_.batch_size * 3 * param_.h * param_.w * sizeof(float)));
    CHECK(cudaMallocHost(&buffers_[output_stride8], param_.batch_size * 3*80*80*29   * sizeof(float)));
    CHECK(cudaMallocHost(&buffers_[output_stride16], param_.batch_size * 3*40*40*29   * sizeof(float)));
    CHECK(cudaMallocHost(&buffers_[output_stride32], param_.batch_size * 3*20*20*29   * sizeof(float)));
    
    CHECK(cudaStreamCreate(&stream_));
}

void toe::Detector::preprocess()
{
    ppreprocess(input_img, buffers_[inputIndex]
        , 640, 640, dstDevData, midDevData, srcDevData_left, 640, 640);
    cudaDeviceSynchronize();
}

void toe::Detector::inference()
{
    context_->executeV2(buffers_);
    cudaStreamSynchronize(stream_);
}

void toe::Detector::postprocess()
{
    auto start = std::chrono::high_resolution_clock::now();
    float* left_ptr = (float*)(buffers_[output_stride8]);
    float* right_ptr = (float*)(buffers_[output_stride8])+3*80*80*29;
    int nums[1];
    int nums_right[1];
    nums_right[0] = 0;
    nums[0] = 0;
    cudaMemcpy(outputs_decode_nums, nums, sizeof(int), cudaMemcpyHostToDevice);
    cudaMemcpy(outputs_decode_nums_right, nums, sizeof(int), cudaMemcpyHostToDevice);
    cudaDeviceSynchronize();

    decode_outputs_cu(left_ptr, outputs_decode,
                8, 29, 640, 640, param_, outputs_decode_nums);
    decode_outputs_cu(right_ptr, outputs_decode_right,
                8, 29, 640, 640, param_, outputs_decode_nums_right);

    cudaDeviceSynchronize();
    
    left_ptr = (float*)(buffers_[output_stride16]);
    right_ptr = (float*)(buffers_[output_stride16])+3*40*40*29;
    decode_outputs_cu(left_ptr, outputs_decode,
                    16, 29, 640, 640, param_ ,outputs_decode_nums);
    decode_outputs_cu(right_ptr, outputs_decode_right,
                    16, 29, 640, 640, param_ ,outputs_decode_nums_right);
    cudaDeviceSynchronize();
    
    left_ptr = (float*)(buffers_[output_stride32]);
    right_ptr = (float*)(buffers_[output_stride32])+3*20*20*29;
    decode_outputs_cu(left_ptr, outputs_decode,
                    32, 29, 640, 640, param_ ,outputs_decode_nums);
    decode_outputs_cu(right_ptr, outputs_decode_right,
                    32, 29, 640, 640, param_ ,outputs_decode_nums_right);
    cudaDeviceSynchronize();
    
    cudaMemcpy(nums, outputs_decode_nums, sizeof(int), cudaMemcpyDeviceToHost);
    cudaMemcpy(nums_right, outputs_decode_nums_right, sizeof(int), cudaMemcpyDeviceToHost);
    int ori_nums = *nums;
    int ori_nums_right = *nums_right;

    cudaDeviceSynchronize();
    do_nms(outputs_decode, final_out, outputs_decode_nums,  param_, *nums);
    cudaDeviceSynchronize();
    do_nms(outputs_decode_right, final_out_right, outputs_decode_nums_right,  param_, *nums_right);
    cudaDeviceSynchronize();
    cudaMemcpy(nums, outputs_decode_nums, sizeof(int), cudaMemcpyDeviceToHost);
    cudaMemcpy(nums_right, outputs_decode_nums_right, sizeof(int), cudaMemcpyDeviceToHost);
    
    show_left = input_img.clone();
    show_right = input_img.clone();
    float shit[360];
    cudaMemcpy(shit, final_out, 20*  18* sizeof(float), cudaMemcpyDeviceToHost);
    for (auto n=0;n<*nums;n++)
    {
        
        for (int p=0;p<5;p++)
        {
            float x_c = shit[n*18+p*2];
            float y_c = shit[n*18+p*2+1];
            // cout << x_c << " " << y_c << endl;
            circle(show_left, cv::Point(int(x_c),int(y_c)), 2, cv::Scalar(0,0,255), -1);
        }
        
        //float x_c = (shit[n*18+14]+shit[n*18+16])/2;
        //float y_c = (shit[n*18+15]+shit[n*18+17])/2;
        //circle(input_img_, Point(int(x_c),int(y_c)), 2, Scalar(0,0,255), -1);
        cv::rectangle(show_left,cv::Point(int(shit[n*18+14]),int(shit[n*18+15])),
            cv::Point(int(shit[n*18+16]),int(shit[n*18+17])),cv::Scalar(0,255,0),1,1,0);
    }
    
    cudaDeviceSynchronize();
    float shit1[360];
    cudaMemcpy(shit1, final_out_right, 20*  18* sizeof(float), cudaMemcpyDeviceToHost);
    for (auto n=0;n<*nums_right;n++)
    {
        
        for (int p=0;p<5;p++)
        {
            float x_c = shit1[n*18+p*2];
            float y_c = shit1[n*18+p*2+1];
            cv::circle(show_right, cv::Point(int(x_c),int(y_c)), 2, cv::Scalar(0,0,255), -1);
        }
        
        cv::rectangle(show_right,cv::Point(int(shit1[n*18+14]),int(shit1[n*18+15])),
            cv::Point(int(shit1[n*18+16]),int(shit1[n*18+17])),cv::Scalar(0,255,0),1,1,0);
    }
    cudaDeviceSynchronize();
}

#endif