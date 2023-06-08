/*
* Detector.cpp
* Created on: 20230605
* Author: sumang
* Descriptron: armor detector
*/

#include <iostream>
#include "detector.h"
#include "common_structs.h"

using namespace std;
using namespace cv;

void MyDetector::Init(Appconfig* config)
{
#ifdef USE_NVIDIA
    param_.engine_file_path = config->detect_config.engine_file_path;
#else
    param_.bin_file_path = config->detect_config.bin_file_path;
    param_.xml_file_path = config->detect_config.xml_file_path;
#endif 

    param_.batch_size = config->detect_config.batch_size;
    param_.c = config->detect_config.c;
    param_.w = config->detect_config.w;
    param_.h = config->detect_config.h;

    param_.type = config->detect_config.type;
    param_.width = config->detect_config.width;
    param_.height = config->detect_config.height;

    param_.nms_thresh = config->detect_config.nms_thresh;
    param_.bbox_conf_thresh = config->detect_config.bbox_conf_thresh;
    param_.merge_thresh = config->detect_config.merge_thresh;

    param_.sizes = config->detect_config.sizes;
    param_.colors = config->detect_config.colors;
    param_.classes = config->detect_config.classes;
    
}

void MyDetector::push_img(Mat& img)
{
    img_mutex_.lock();
    if (input_imgs_.size() == max_size_) input_imgs_.clear();
    input_imgs_.emplace_back(img);
    img_mutex_.unlock();
}