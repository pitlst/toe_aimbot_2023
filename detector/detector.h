/*
* Detector.h
* Created on: 20230605
* Author: sumang
* Description: armor detector
*/
#ifndef DETECTOR_H_
#define DETECTOR_H_

#include <opencv2/opencv.hpp>
#include <mutex>
#include "../common_structs.h"

class MyDetector
{
protected:
    cv::Mat input_img_;
    std::mutex img_mutex_;
    s_detector_params param_;
    s_detections outputs_;

public:
    void show_results();
    void get_results();

public:
    MyDetector() = default;
    ~MyDetector();

    void Init(Appconfig* config);
    virtual void preprocess() = 0;
    virtual void inference() = 0;
    virtual void postprocess() = 0;

}

#endif