#include <iostream>
#include "debug.hpp"
#include "grab_img.hpp"

int main()
{
    toe::hik_camera temp;
    toe::json_head temp_apra;
    temp_apra.open(std::string(PROJECT_PATH) + std::string("/data/setting.json"));
    temp.hik_init(temp_apra,0);
    int k = 0;
    cv::Mat img;
    while (k != 27)
    {
        frame_mutex.lock();
        img = frame_rgb;
        frame_mutex.unlock();
        if (img.data)
        {
            cv::imshow("frame",img);
        }
        k = cv::waitKey(1);
    }
    temp.hik_end();
    return 0;
}