/*
* ov_detector.h
* Created on: 20230605
* Author: sumang
* Description: armor detector of openvino
*/
#ifndef OV_DETECT_H_
#define OV_DETECT_H_

#ifdef USE_NVIDIA
#else 
#include <inference_engine.hpp>
#include "detector.h"
#include "../../common_structs.h"

class OvO_Detector:MyDetector 
{
private:
    InferenceEngine::Core ie_;
    InferenceEngine::CNNNetwork network_;
    InferenceEngine::ExecutableNetwork executable_network_;
    InferenceEngine::InferRequest infer_request_;
    std::vector<float> blob(640*640*3);
    std::string input_name_;
    std::vector<ml::OutLayer> output_layers_;
    std::vector<std::string> output_names_;
    std::vector<float> anchors_[4];
    void copyBlob(Data& blob, InferenceEngine::Blob::Ptr& ieBlob);
    void preprocess() override;
    void inference() override;
    void postprocess() override;
    bool detect();

public:
    OvO_Detector();
    ~OvO_Detector();

}

#endif

#endif