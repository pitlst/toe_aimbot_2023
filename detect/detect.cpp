#include "detect.hpp"

#include <iostream>

void toe::Detector_base::Init(const toe::json_head & input_json, int color)
{
    toe::json temp_json = input_json;

    param_.engine_file_path = temp_json["path"]["engine_file_path"].String();
    param_.bin_file_path = temp_json["path"]["bin_file_path"].String();
    param_.xml_file_path = temp_json["path"]["xml_file_path"].String();

    param_.camp = color;
#ifdef USE_NVIDIA
    param_.batch_size = temp_json["NCHW"]["orin_nx"]["batch_size"].Int();
    param_.c = temp_json["NCHW"]["orin_nx"]["C"].Int();
    param_.w = temp_json["NCHW"]["orin_nx"]["W"].Int();
    param_.h = temp_json["NCHW"]["orin_nx"]["H"].Int();
#else
    param_.batch_size = temp_json["NCHW"]["openvino"]["batch_size"].Int();
    param_.c = temp_json["NCHW"]["openvino"]["C"].Int();
    param_.w = temp_json["NCHW"]["openvino"]["W"].Int();
    param_.h = temp_json["NCHW"]["openvino"]["H"].Int();
#endif
    param_.width = temp_json["camera"]["0"]["width"].Int();
    param_.height = temp_json["camera"]["0"]["height"].Int();

    param_.nms_thresh = temp_json["thresh"]["nms_thresh"].Double();
    param_.bbox_conf_thresh = temp_json["thresh"]["bbox_conf_thresh"].Double();
    param_.merge_thresh = temp_json["thresh"]["merge_thresh"].Double();

    param_.classes = temp_json["nums"]["classes"].Int();
    param_.sizes = temp_json["nums"]["sizes"].Int();
    param_.colors = temp_json["nums"]["colors"].Int();
    param_.kpts = temp_json["nums"]["kpts"].Int();

    std::vector<float> temp_vector;
    for (toe::json ch : temp_json["anchors"]["1"])
    {
        temp_vector.emplace_back(ch.Int());
    }
    param_.a1 = temp_vector;
    temp_vector.clear();
    for (toe::json ch : temp_json["anchors"]["2"])
    {
        temp_vector.emplace_back(ch.Int());
    }
    param_.a2 = temp_vector;
    temp_vector.clear();
    for (toe::json ch : temp_json["anchors"]["3"])
    {
        temp_vector.emplace_back(ch.Int());
    }
    param_.a3 = temp_vector;
    temp_vector.clear();
    for (toe::json ch : temp_json["anchors"]["4"])
    {
        temp_vector.emplace_back(ch.Int());
    }
    param_.a4 = temp_vector;
    temp_vector.clear();
}

void toe::Detector_base::push_img(const cv::Mat& img)
{   
    img_mutex_.lock();
    if (input_imgs.size() >= max_size_)
    {   
        input_imgs.clear();
    } 
    input_imgs.emplace_back(img.clone());
    img_mutex_.unlock();
}

bool toe::Detector_base::show_results(cv::Mat& img)
{
    cv::resize(img, img, cv::Size(640,640));
    cv::Point ct0, ct1, ct2, ct3;
    for (auto i = 0; i < outputs_armor.size(); i++)
    {
        ct0 = cv::Point(int(outputs_armor[i].pts[0].x), int(outputs_armor[i].pts[0].y));
        ct1 = cv::Point(int(outputs_armor[i].pts[1].x), int(outputs_armor[i].pts[1].y));
        ct2 = cv::Point(int(outputs_armor[i].pts[2].x), int(outputs_armor[i].pts[2].y));
        ct3 = cv::Point(int(outputs_armor[i].pts[3].x), int(outputs_armor[i].pts[3].y));

        cv::line(img, ct0, ct1, cv::Scalar(128,255,128), 2);
        cv::line(img, ct1, ct2, cv::Scalar(128,255,128), 2);
        cv::line(img, ct2, ct3, cv::Scalar(128,255,128), 2);
        cv::line(img, ct3, ct0, cv::Scalar(128,255,128), 2);

        std::string armor_text; // b r n p
        if(outputs_armor[i].color == 0) armor_text += "b";
        else if(outputs_armor[i].color == 1) armor_text += "r";
        else if(outputs_armor[i].color == 2) armor_text += "n";
        else if(outputs_armor[i].color == 3) armor_text += "p";

        armor_text += std::to_string(outputs_armor[i].type);

        if(outputs_armor[i].t_size == 1) armor_text += "*";
        
        cv::putText(img, armor_text, ct0, cv::FONT_HERSHEY_PLAIN, 4, cv::Scalar(0,255,0), 1);
    }
    return true;
}

std::vector<armor_data> toe::Detector_base::get_results()
{
    std::vector<armor_data> temp_return;
    outputs_mutex_.lock();
    temp_return = outputs_armor;
    outputs_mutex_.unlock();
    return temp_return;
}

bool toe::Detector_base::detect()
{
    if (!input_imgs.empty())
    {
        // std::cout << "read" << std::endl;
        img_mutex_.lock();
        input_img = input_imgs.back();
        input_imgs.clear();
        img_mutex_.unlock();

        // std::cout << "trans" << std::endl;
        preprocess();
        // std::cout << "infer" << std::endl;
        inference();
        // std::cout << "post" << std::endl;
        postprocess();
    }
    return true;
}