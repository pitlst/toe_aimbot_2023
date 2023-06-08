/*
* Detector.cpp
* Created on: 20230605
* Author: sumang
* Descriptron: armor detector
*/

#include <iostream>
#include "detector.h"
#include "../common_structs.h"

using namespace std;
using namespace cv;

void MyDetector::Init(Appconfig* config)
{
#ifdef USE_NVIDIA
    param_.path.engine_file_path = ;
#else
    param_.path.bin_file_path = ;
    param_.path.xml_file_path = ;
#endif 

    param_.NCHW.batch_size = ;
    param_.NCHW.c = ;
    param_.NCHW.w = ;
    param_.NCHW.h = ;

    param_.img.type = ;
    param_.img.width = ;
    param_.img.height = ;

    param_.thresh.nms_thresh = ;
    param_.thresh.bbox_conf_thresh = ;
    param_.thresh.merge_thresh = ;

    param_.nums.sizes = ;
    param_.nums.colors = ;
    param_.nums.classes = ;

    
}

void MyDetector::push_img(Mat& img)
{
    img_mutex_.lock();
    if (input_imgs_.size() == max_size_) input_imgs_.clear();
    input_imgs_.emplace_back(img);
    img_mutex_.unlock();
}