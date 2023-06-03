#pragma once
#include <string>
#include <sstream>

#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>

#include "/opt/MVS/include/MvCameraControl.h"

#include "logger.h"
#include "toe_json.h"

namespace toe
{
    inline bool is_Numeric(std::string str)
    {
        for (auto &ch : str)
        {
            if (ch < '0' || ch > '9')
            {
                return false;
            }
        }
        return true;
    }

    class GetFrame final
    {
    public:
        GetFrame();
        GetFrame(const std::string &source_path);
        ~GetFrame();

        void set(const std::string &source_path);
        void restart_camera();
        cv::Mat &GetOneFrame();
        void EndCamera();

        struct camera_data
        {
            int width;
            int height;
            int exposure_time;
            int offsetX;
            int offsetY;
            int max_video_time;
        };
        struct realsence_data
        {
            int width;
            int height;
            int fps_rgb;
            int fps_depth;
        };
        camera_data m_camera;
        realsence_data m_d435;

        // 标志位，相机是否正常开启
        bool open_label = false;

    private:
        void read_json(const std::string &input_filename);
        void StartCamera();

        // 阻止构造一些常用的特定重载函数
        GetFrame operator<<(const GetFrame &) = delete;
        GetFrame operator>>(const GetFrame &) = delete;
        GetFrame operator=(const GetFrame &) = delete;
        GetFrame operator+(const GetFrame &) = delete;
        GetFrame operator-(const GetFrame &) = delete;
        GetFrame operator*(const GetFrame &) = delete;
        GetFrame operator/(const GetFrame &) = delete;
        GetFrame operator++() = delete;
        GetFrame operator--() = delete;
        GetFrame operator&(const GetFrame &) = delete;

        // 模式标志位
        int video_debug_set;
        std::string source;
        std::string filename;
        cv::VideoCapture capture;
        json_head file;
        cv::Mat frame;
#ifdef SAVE_VIDEO
       cv::VideoWriter video_vapture;
       std::chrono::_V2::system_clock::time_point start_time;
#endif
        // 海康相机的相关函数

        // 打印相机相关信息
        bool PrintDeviceInfo(MV_CC_DEVICE_INFO *pstMVDevInfo);
        // 帧数据转换为Mat格式图片并保存
        void Convert2Mat();

        //海康相机需要的相关参数

        // 海康相机指针
        void *handle = nullptr;
        // 帧数据指针
        unsigned char *pData;
        // 帧数据大小
        unsigned int nDataSize;
        // 输出帧的信息
        MV_FRAME_OUT_INFO_EX stImageInfo = {0};
        // 设备信息
        MV_CC_DEVICE_INFO *pDeviceInfo;
    };
}
