#include <iostream>
#include <string>
#include <thread>

#include "toe_json.hpp"
#include "toe_serial.hpp"
#include "grab_img.hpp"
#include "detect.hpp"
#include "ov_detect.hpp"

toe::hik_camera hik_cam;
toe::OvO_Detector ov_detector;
toe::json_head config;

int mode = 0;
int color = 0;

void detect_process(void)
{
    ov_detector.Init(config, color);
    ov_detector.openvino_init();
    while (1)
    {
        ov_detector.detect();
    }
}

void grab_img(void)
{
    hik_cam.hik_init(config, 0);
    // 防止不正常退出后启动异常
    hik_cam.hik_end();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    hik_cam.hik_init(config, 0);
    int k = 0;
    while (k != 27)
    {
        frame_mutex.lock();
        cv::Mat frame = frame_rgb;
        frame_mutex.unlock();
        if(frame.size[0] > 0)
        {
            ov_detector.push_img(frame);
            ov_detector.show_results(frame);
            cv::imshow("1", frame);
            k = cv::waitKey(1);
        }
    }
    hik_cam.hik_end();
    cv::destroyAllWindows();
}

int main()
{
    std::cout << PROJECT_PATH << std::endl;
    config.open(std::string(PROJECT_PATH) + std::string("/data/setting.json"));
    std::thread grab_thread = std::thread(grab_img);
    grab_thread.detach();
    std::thread detect_thread = std::thread(detect_process);
    detect_thread.detach();
    while(1){;}
    return 0;
}