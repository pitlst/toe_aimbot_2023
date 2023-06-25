#include "detect.hpp"

#include <iostream>

void toe::Detector::Init(const toe::json_head & input_json, int color)
{
    toe::json temp_json = input_json;

    param_.engine_file_path = temp_json["path"]["engine_file_path"].String();
    param_.bin_file_path = temp_json["path"]["bin_file_path"].String();
    param_.xml_file_path = temp_json["path"]["xml_file_path"].String();

    param_.camp = color;
    param_.batch_size = temp_json["NCHW"]["batch_size"].Int();
    param_.c = temp_json["NCHW"]["C"].Int();
    param_.w = temp_json["NCHW"]["W"].Int();
    param_.h = temp_json["NCHW"]["H"].Int();

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

void toe::Detector::push_img(const cv::Mat& img)
{   
    img_mutex_.lock();
    if (input_imgs.size() == max_size_)
    {   
        input_imgs.clear();
    } 
    input_imgs.emplace_back(img.clone());
    img_mutex_.unlock();
}

// armor_data toe::Detector::get_results(std::vector<armor_data>& armor)
// {
//     armor_data rst;
//     rst.x_c = -1;
//     rst.y_c = -1;
//     rst.z = -1;
//     rst.type = 0;
//     if (armor.size() == 0) return rst;
//     // 1 > 345 > 0 > 2 > 67
//     int x_c = param_.width/2;
//     int y_c = param_.height/2;
//     std::vector<armor_data> result[8];
//     for (int i=0; i<armor.size(); i++)
//         result[armor[i].type].emplace_back(armor[i]);

//     if (result[1].size() > 0)
//     {
//         if (result[1].size()==1) rst = result[1][0];
//         else
//         {
//             int idx;
//             int min_dis = 99999;
//             for (int j=0;j<result[1].size();j++)
//             {
//                 if (abs(result[1][j].x_c-int(x_c)) < min_dis)
//                 {
//                     min_dis = abs(result[1][j].x_c-int(x_c));
//                     idx = j;
//                 }
//             }
//             rst = result[1][idx];
//         }
//     }
//     else if(result[3].size()+result[4].size()+result[5].size() > 0)
//     {
//         int idxx;
//         int idx;
//         int min_dis = 99999;
//         for (int j=0;j<result[3].size();j++)
//         {
//             if (abs(result[3][j].x_c-int(x_c)) < min_dis)
//             {
//                 min_dis = abs(result[3][j].x_c-int(x_c));
//                 idx = j;
//                 idxx = 3;
//             }
//         }
//         for (int j=0;j<result[5].size();j++)
//         {
//             if (abs(result[5][j].x_c-int(x_c)) < min_dis)
//             {
//                 min_dis = abs(result[5][j].x_c-int(x_c));
//                 idx = j;
//                 idxx = 5;
//             }
//         }
//         for (int j=0;j<result[4].size();j++)
//         {
//             if (abs(result[4][j].x_c-int(x_c)) < min_dis)
//             {
//                 min_dis = abs(result[4][j].x_c-int(x_c));
//                 idx = j;
//                 idxx = 4;
//             }
//         }
//         rst = result[idxx][idx];
//     }
//     else if(result[0].size() > 0)
//     {
//         if (result[0].size()==1) rst = result[0][0];
//         else
//         {
//             int idx;
//             int min_dis = 99999;
//             for (int j=0;j<result[0].size();j++)
//             {
//                 if (abs(result[0][j].x_c-int(x_c)) < min_dis)
//                 {
//                     min_dis = abs(result[0][j].x_c-int(x_c));
//                     idx = j;
//                 }
//             }
//             rst = result[0][idx];
//         }
//     }
//     else if(result[2].size() > 0)
//     {
//         if (result[2].size()==1) rst = result[2][0];
//         else
//         {
//             int idx;
//             int min_dis = 99999;
//             for (int j=0;j<result[2].size();j++)
//             {
//                 if (abs(result[2][j].x_c-int(x_c)) < min_dis)
//                 {
//                     min_dis = abs(result[2][j].x_c-int(x_c));
//                     idx = j;
//                 }
//             }
//             rst = result[2][idx];
//         }
//     }
//     else if(result[6].size() > 0)
//     {
//         if (result[6].size()==1) rst = result[6][0];
//         else
//         {
//             int idx;
//             int min_dis = 99999;
//             for (int j=0;j<result[6].size();j++)
//             {
//                 if (abs(result[6][j].x_c-int(x_c)) < min_dis)
//                 {
//                     min_dis = abs(result[6][j].x_c-int(x_c));
//                     idx = j;
//                 }
//             }
//             rst = result[6][idx];
//         }
//     }

//     cv::Point2f left_bottom = rst.pts[0];
//     cv::Point2f left_top = rst.pts[1];
//     cv::Point2f right_bottom = rst.pts[2];
//     cv::Point2f right_top = rst.pts[3];

//     double l_length = sqrt(pow(left_bottom.x*param_.width/640-left_top.x*param_.width/640, 2) 
//             + pow(left_bottom.y*param_.height/640-left_top.y*param_.height/640, 2));
    
//     double r_length = sqrt(pow(left_bottom.x*param_.width/640-right_bottom.x*param_.width/640, 2) 
//             + pow(left_bottom.y*param_.height/640-right_bottom.y*param_.height/640, 2));


//     if ((l_length+r_length) > 0)
//         rst.z = param_.z_scale / (l_length+r_length);
//     else
//         rst.z = 999;

//     return rst;
// }

bool toe::Detector::show_results(cv::Mat& img)
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