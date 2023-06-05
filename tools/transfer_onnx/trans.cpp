#include <fstream>
#include <iostream>
#include <sstream>

#include "NvInfer.h"
#include "NvOnnxParser.h"
//#include "NvinferRuntime.h"

using namespace nvinfer1;
using namespace nvonnxparser;

// 全局创建 ILogger 类型的对象
class Logger : public ILogger
{
    virtual void log(Severity severity, const char* msg) noexcept override
    {
        // suppress info-level messages
        if (severity != Severity::kINFO)
            std::cout << msg << std::endl;
    }
} gLogger;

int onnx2engine(std::string onnx_filename, std::string enginefilePath, int type){


    // 创建builder
    IBuilder* builder = createInferBuilder(gLogger);

    // 创建network
    nvinfer1::INetworkDefinition* network = builder->createNetworkV2(1U << static_cast<uint32_t>(NetworkDefinitionCreationFlag::kEXPLICIT_BATCH));

    // 创建onnx模型解析器
    auto parser = nvonnxparser::createParser(*network, gLogger);

    // 解析模型
    parser->parseFromFile(onnx_filename.c_str(), 2);
    for (int i = 0; i < parser->getNbErrors(); ++i)
    {
        std::cout << parser->getError(i)->desc() << std::endl;
    }
    printf("tensorRT load onnx model sucessful! \n"); // 解析模型成功，第一个断点测试


    // 使用builder对象构建engine
    IBuilderConfig* config = builder->createBuilderConfig();
    config->setMaxWorkspaceSize(16 * (1 << 20));  // 设置最大工作空间
    config->setFlag(BuilderFlag::kGPU_FALLBACK);  // 启用GPU回退模式，作用？
    //	config->setFlag(BuilderFlag::kSTRICT_TYPES);  //强制执行xx位的精度计算
    if (type == 1) {
        config->setFlag(nvinfer1::BuilderFlag::kFP16); // 设置精度计算
    }
    if (type == 2) {
        config->setFlag(nvinfer1::BuilderFlag::kINT8);
    }

    IOptimizationProfile* profile = builder->createOptimizationProfile(); //创建优化配置文件
    profile->setDimensions("x", OptProfileSelector::kMIN, Dims4(1, 3, 640, 640)); // 设置输入x的动态维度，最小值
    profile->setDimensions("x", OptProfileSelector::kOPT, Dims4(1, 3, 640, 640)); // 期望输入的最优值
    profile->setDimensions("x", OptProfileSelector::kMAX, Dims4(1, 3, 640, 640)); // 最大值
    config->addOptimizationProfile(profile);


    nvinfer1::IHostMemory *myengine = builder->buildSerializedNetwork(*network, *config);

//    ICudaEngine* myengine = builder->buildEngineWithConfig(*network, *config); //创建engine  第二个断点测试
    std::cout << "try to save engine file now" << std::endl;
    std::ofstream p(enginefilePath, std::ios::binary);
    if (!p) {
        std::cerr << "could not open plan output file" << std::endl;
        return 0;
    }

    // 序列化
//    IHostMemory* modelStream = myengine->serialize(); // 第三个断点测试
    p.write(reinterpret_cast<const char*>(myengine->data()), myengine->size());
//    p.write(reinterpret_cast<const char*>(modelStream->data()), modelStream->size()); // 写入
//	modelStream->destroy(); // 销毁
//	myengine->destroy();
//	network->destroy();
//	parser->destroy();
//    delete modelStream;
    delete myengine;
    delete network;
    delete parser;
    std::cout << "convert onnx model to TensorRT engine model successfully!" << std::endl; // 转换成功，第四个断点测试

    return 0;
}

int main() {

    onnx2engine("yolov5n.onnx", \
                "yolov5n.engine", 1);
    return 0;
}

