#include <iostream>
#include <opencv2/opencv.hpp>
#include <thread>

#include "string.h"

#include "debug.hpp"
#include "grab_img.hpp"
#include "detect.hpp"
#include "ov_detect.hpp"

int main()
{
    toe::hik_camera temp;
    toe::json_head temp_apra;
    toe::Detector ov_detector;

    temp_apra.open(std::string(PROJECT_PATH) + std::string("/data/setting.json"));
    temp.hik_init(temp_apra,0);
    ov_detector.Init(temp_apra, 0);
    int k = 0;
    cv::Mat img;
    while (k != 27)
    {
        frame_mutex.lock();
        img = frame_rgb;
        frame_mutex.unlock();
        if (img.data)
        {
            ov_detector.push_img(img);
            ov_detector.show_results(img);
            cv::imshow("frame",img);
        }
        k = cv::waitKey(1);
    }
    temp.hik_end();
    cv::destroyAllWindows();
    return 0;
}