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
        std::vector<cv::Mat> input_imgs_;
        const int max_size_ = 10;
        detect_data param_;
        std::vector<armor_data> outputs_armor;
        std::mutex img_mutex_;
        std::mutex outputs_mutex_;

    public:
        cv::Mat input_img_;
    }; 
}

#endif