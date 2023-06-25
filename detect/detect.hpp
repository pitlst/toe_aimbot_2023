#ifndef DETECT_H_
#define DETECT_H_

#include <opencv2/opencv.hpp>
#include <mutex>
#include <vector>

#include "debug.hpp"
#include "toe_structs.hpp"
#include "toe_json.hpp"


namespace toe
{
    class Detector
    {    
    public:
        Detector() = default;
        ~Detector() = default;

        virtual void preprocess() = 0;
        virtual void inference() = 0;
        virtual void postprocess() = 0;

        void Init(const toe::json_head & input_json, int color);
        void push_img(const cv::Mat& img);
        bool show_results(cv::Mat& img);
        armor_data get_results(std::vector<armor_data>& armor);

    protected:
        // 输入的图像缓存
        const int max_size_ = 10;
        std::vector<cv::Mat> input_imgs;
        
        // 配置参数
        detect_data param_;
        // 检测到的的装甲板
        std::vector<armor_data> outputs_armor;
        // 输入图像的线程锁
        std::mutex img_mutex_;
        // 输出装甲板的线程锁
        std::mutex outputs_mutex_;

    public:
        cv::Mat input_img;
    }; 
}

#endif