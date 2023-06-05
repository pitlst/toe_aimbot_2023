#include "NvInfer.h"
#include "NvOnnxParser.h"
#include "logger.h"
#include "common.h"
#include "buffers.h"
#include <opencv2/opencv.hpp>


#define BATCH_SIZE 1
#define INPUT_H 640
#define INPUT_W 640
#define OUTPUT_SIZE_8 1632000
#define OUTPUT_SIZE_16 408000
#define OUTPUT_SIZE_32 102000
#define OUTPUT_SIZE_E320 535500
#define OUTPUT_SIZE_G640 2142000

using namespace std;


// 加载模型文件
std::vector<unsigned char> load_engine_file(const std::string &file_name)
{
    std::vector<unsigned char> engine_data;
    std::ifstream engine_file(file_name, std::ios::binary);
    assert(engine_file.is_open() && "Unable to load engine file.");
    engine_file.seekg(0, engine_file.end);
    int length = engine_file.tellg();
    engine_data.resize(length);
    engine_file.seekg(0, engine_file.beg);
    engine_file.read(reinterpret_cast<char *>(engine_data.data()), length);
    return engine_data;
}
/*
void doInference(IExecutionContext& context, cudaStream_t& stream, void **buffers, float* input, float* output, int batchSize) {
    // DMA input batch data to device, infer on the batch asynchronously, and DMA output back to host
    context.enqueue(batchSize, buffers, stream, nullptr);
    cudaStreamSynchronize(stream);
}
*/
int main(int argc, char **argv)
{

    auto engine_file = "./yolov5n.engine";      // 模型文件
    auto img_path = "./533.jpg"; // 输入视频文件

    // ========= 1. 创建推理运行时runtime =========
    auto runtime = std::unique_ptr<nvinfer1::IRuntime>(nvinfer1::createInferRuntime(sample::gLogger.getTRTLogger()));
    if (!runtime)
    {
        std::cout << "runtime create failed" << std::endl;
        return -1;
    }
    // ======== 2. 反序列化生成engine =========
    // 加载模型文件
    auto plan = load_engine_file(engine_file);
    // 反序列化生成engine
    auto engine = std::shared_ptr<nvinfer1::ICudaEngine>(runtime->deserializeCudaEngine(plan.data(), plan.size()));
    if (!engine)
    {
        return -1;
    }

    // ======== 3. 创建执行上下文context =========
    auto context = std::unique_ptr<nvinfer1::IExecutionContext>(engine->createExecutionContext());
    if (!context)
    {
        std::cout << "context create failed" << std::endl;
        return -1;
    }


    cv::Mat img;
    int frame_index{0};


    img = cv::imread("533.jpg");
    cv::resize(img, img, cv::Size(1920,1080));
    
    static float pre_data[BATCH_SIZE*3*INPUT_H*INPUT_W];
    unsigned char* pre_input;
    static float data[BATCH_SIZE * 3 * INPUT_H * INPUT_W];  //输入
    static float prob[BATCH_SIZE * (OUTPUT_SIZE_8+OUTPUT_SIZE_16+OUTPUT_SIZE_32)];            //输出

    
    void* buffers[5];
    const int inputIndex = engine->getBindingIndex("images");
    const int outputIndex = engine->getBindingIndex("output");
    const int outputIndex_stride_8 = engine->getBindingIndex("350");
    const int outputIndex_stride_16 = engine->getBindingIndex("416");
    const int outputIndex_stride_32 = engine->getBindingIndex("482");

    
    // Create GPU buffers on device
    CHECK(cudaMalloc(&pre_input, BATCH_SIZE * 3 * INPUT_H * INPUT_W * sizeof(float)));
    CHECK(cudaMalloc(&buffers[inputIndex], BATCH_SIZE * 3 * INPUT_H * INPUT_W * sizeof(float)));
    CHECK(cudaMalloc(&buffers[outputIndex], BATCH_SIZE * OUTPUT_SIZE_G640 * sizeof(float)));
    CHECK(cudaMalloc(&buffers[outputIndex_stride_8], BATCH_SIZE * OUTPUT_SIZE_8 * sizeof(float)));
    CHECK(cudaMalloc(&buffers[outputIndex_stride_16], BATCH_SIZE * OUTPUT_SIZE_16 * sizeof(float)));
    CHECK(cudaMalloc(&buffers[outputIndex_stride_32], BATCH_SIZE * OUTPUT_SIZE_32 * sizeof(float)));
        
    // Create stream
    cudaStream_t stream;
    CHECK(cudaStreamCreate(&stream)); 
    
    auto start = std::chrono::high_resolution_clock::now();
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    float sum_pre = 0;
    float sum_infer = 0;
    auto time_str = std::to_string(elapsed) + "ms";


    for(int i=0; i<8000; i++)
    {
        start = std::chrono::high_resolution_clock::now();
        // preprocess
        //cudaMemcpy(pre_input, img.data, img.cols*img.rows*3*sizeof(unsigned char), cudaMemcpyHostToDevice);
        //dim3 block_size(32,32);
        //dim3 grid_size((640+32-1)/32,(640+32-1)/32);
        //bgr2rgb_resize_normalize<<<grid_size, block_size>>>(pre_input, buffers[inputIndex], 640,640,640,640);
        
        /*
        cv::resize(img, img, cv::Size(640,640));
        if (!img.empty())
        {
            cv::Mat pr_img;
            cv::cvtColor(img, pr_img, cv::COLOR_BGR2RGB);
            int i = 0;
            for (int row = 0; row < INPUT_H; ++row) {
                uchar* uc_pixel = pr_img.data + row * pr_img.step;
                for (int col = 0; col < INPUT_W; ++col) {
                    data[i] = (float)uc_pixel[2] / 255.0;
                    data[i + INPUT_H * INPUT_W] = (float)uc_pixel[1] / 255.0;
                    data[i + 2 * INPUT_H * INPUT_W] = (float)uc_pixel[0] / 255.0;
                    uc_pixel += 3;
                    ++i;
                }
            }
        }
        */
        end = std::chrono::high_resolution_clock::now();
        elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        if (i>3000)
        sum_pre += elapsed;
        //time_str = std::to_string(elapsed) + "ms";
         
        // Run inference
        start = std::chrono::high_resolution_clock::now();
        doInference(*context, stream, buffers, data, prob, BATCH_SIZE);
        end = std::chrono::high_resolution_clock::now();
        elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        //time_str = std::to_string(elapsed) + "ms";
        //cout << time_str << endl;
        if (i > 3000)
        sum_infer += elapsed;

    }
    cout << "preprocess average: " << sum_pre/5000 << "ms" << endl;
    cout << "infer_average: " << sum_infer/5000 << "ms" << endl;
    cout << "fps:  " << 1000/(sum_infer/5000+sum_infer/5000) << endl;

    return 0;
}
