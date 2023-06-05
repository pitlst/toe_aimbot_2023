/*
* Detector.h
* Created on: 20230605
* Author: sumang
* Description: armor detector of openvino
*/
#ifndef OV_DETECT_H_
#define OV_DETECT_H_

#ifdef USE_NVIDIA
#else 
#include <inference_engine.hpp>
#include "../common_structs.h"

class OvO_Detector:MyDetector 
{
private:
    InferenceEngine::Core ie;
    InferenceEngine::CNNNetwork network;
    InferenceEngine::ExecutableNetwork executable_network;
    InferenceEngine::InferRequest infer_request;
    void copyBlob(Data& blob, InferenceEngine::Blob::Ptr& ieBlob);
    void preprocess() override {};
    void inference() override {};
    void postprocess() override {};

public:
    OvO_Detector() = default;
    ~OvO_Detector();

}

#endif

#endif