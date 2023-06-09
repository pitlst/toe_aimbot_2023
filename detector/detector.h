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
#include <vector>
#include "../common_structs.h"

class MyDetector
{
protected:
    std::vector<cv::Mat> input_imgs_;
    cv::Mat input_img_;
    const int max_size_ = 10;
    std::mutex img_mutex_;
    s_detector_params param_;
    s_detections outputs_;
    std::mutex outputs_mutex_;
    
public:
    void show_results();
    void get_results();
    void push_img(cv::Mat&);

public:
    MyDetector() = default;
    ~MyDetector(){};

    void Init(Appconfig* config);
    virtual void preprocess() = 0;
    virtual void inference() = 0;
    virtual void postprocess() = 0;

};

extern void load_config(Appconfig& config, std::string json_file_path);

#endif
