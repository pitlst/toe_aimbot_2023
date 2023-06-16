#ifndef OV_DETECT_H_
#define OV_DETECT_H_

#ifndef USE_NVIDIA

#include "detect.hpp"
#include "toe_structs.hpp"

#include "openvino/openvino.hpp"

namespace toe
{
    class OvO_Detector : public Detector
    {
    public:
        std::vector<armor_data> output_nms_;
        OvO_Detector() = default;
        ~OvO_Detector() = default;
        void OvO_Init();
        bool detect();

    private:
        ov::Core ie_;
        InferenceEngine::CNNNetwork network_;
        InferenceEngine::ExecutableNetwork executable_network_;
        InferenceEngine::InferRequest infer_request_;
        std::vector<float> blob; 
        std::string input_name_;
        std::vector<s_OutLayer> output_layers_;
        std::vector<std::string> output_names_;
        std::vector<float> anchors_[4];
        
        void copyBlob(std::vector<float>& blob, InferenceEngine::Blob::Ptr& ieBlob);
        void decode_outputs(const float *prob, s_detections &objects,
                                s_OutLayer& layer_info, const int img_w,
                                const int img_h);
        void do_merge_nms();
        void preprocess() override;
        void inference() override;
        void postprocess() override;
    };

    inline float sigmoid(float x)
    {
        return (1.0 / (1.0 + exp(-x)));
    }
    /**
     * @brief calculate sigmoid values in an array
     *
     * @param src pointer of source array
     * @param dst pointer of destination array
     * @param length number of values
     */
    inline void sigmoid(const float *src, float *dst, int length)
    {
        for (int i = 0; i < length; ++i)
        {
            dst[i] = (1.0 / (1.0 + exp(-src[i])));
        }
    }
}

#endif
#endif