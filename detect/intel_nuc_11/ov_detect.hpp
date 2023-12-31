#ifndef OV_DETECT_H_
#define OV_DETECT_H_

#ifndef USE_NVIDIA

#include <vector>

#include "detect.hpp"
#include "general.hpp"

#include "openvino/openvino.hpp"

namespace toe
{
    class Detector final : public Detector_base 
    {
    public:
        Detector() = default;
        ~Detector() = default;
        void Init(const toe::json_head & input_json, int color);

    private:
        void preprocess() override;
        void inference() override;
        void postprocess() override;

    private:
        // openvino推理相关
        ov::Core core;
        ov::InferRequest infer_request;
        // 网络检测到的装甲板
        std::vector<armor_data> output_nms_;
        // anchors和stride，按照输出顺序排列
        std::vector<float> stride_;
        std::vector<std::vector<float>> anchors; 
        // 输出tensor层数
        size_t out_tensor_size;
        // 准备输入网络的图像数据
        std::vector<float> blob;
        cv::Mat input_temp;
    };

}
#endif
#endif