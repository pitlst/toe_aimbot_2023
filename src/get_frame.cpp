#include "get_frame.h"
#include <chrono>
using namespace toe;

GetFrame::GetFrame()
{
}

GetFrame::GetFrame(const std::string &source_path)
{
    if (source_path == "HIVISION")
    {
        log_debug("来源:海康相机");
        video_debug_set = 0;
    }
    else if (is_Numeric(source_path))
    {
        log_debug("来源:USB相机");
        video_debug_set = 1;
    }
    else
    {
        log_debug("来源:视频");
        video_debug_set = 2;
    }
    source = source_path;
    read_json(PATH_ASSET + "camera.json");
    StartCamera();
#ifdef SAVE_VIDEO
    std::string video_path = PATH_VIDEOWRITER + std::to_string((int)(std::chrono::high_resolution_clock::now().time_since_epoch().count() / TIME_TRANSFORMER)) + ".mp4";
    start_time = std::chrono::high_resolution_clock::now();
    video_vapture.open(video_path, cv::VideoWriter::fourcc('m','p','4','v'), 30.0, cv::Size(m_camera.width, m_camera.height));
    log_info("已开启录像，路径为", video_path);
#endif
}

GetFrame::~GetFrame()
{
    EndCamera();
    frame.release();
    capture.release();
    file.clear();
}

void GetFrame::set(const std::string &source_path)
{
    if (source_path == "HIVISION")
    {
        log_debug("来源:海康相机");
        video_debug_set = 0;
    }
    else if (is_Numeric(source_path))
    {
        log_debug("来源:USB相机");
        video_debug_set = 1;
    }
    else
    {
        log_debug("来源:视频");
        video_debug_set = 2;
    }

    if (open_label)
    {
        restart_camera();
    }
    else
    {
        source = source_path;
        read_json(PATH_ASSET + "camera.json");
        StartCamera();
    }
}

void GetFrame::restart_camera()
{
    EndCamera();
    read_json(filename);
    StartCamera();
}

void GetFrame::read_json(const std::string &input_filename)
{
    file.open(input_filename);
    m_camera.width = file["Aimbot"]["width"];
    m_camera.exposure_time = file["Aimbot"]["exposure_time"];
    m_camera.offsetX = file["Aimbot"]["offsetX"];
    m_camera.height = file["Aimbot"]["height"];
    m_camera.offsetY = file["Aimbot"]["offsetY"];
    m_camera.max_video_time = file["Aimbot"]["max_video_time"];

    file.clear();
}

void GetFrame::StartCamera()
{
    if (video_debug_set == 1)
    {
        capture.open(std::stoi(source), cv::CAP_V4L2);
        if (!capture.isOpened())
        {
            log_error("相机开启失败");
            throw std::logic_error("相机开启失败");
        }
        capture.set(cv::CAP_PROP_FOURCC,cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
        capture.set(cv::CAP_PROP_FRAME_WIDTH, 2560);//设置摄像头采集图像分辨率
        capture.set(cv::CAP_PROP_FRAME_HEIGHT, 720);
        capture.set(cv::CAP_PROP_FPS, 30);
    }
    else if (video_debug_set == 2)
    {
        capture.open(source);
        if (!capture.isOpened())
        {
            log_error("相机开启失败");
            throw std::logic_error("相机开启失败");
        }
    }
    else
    {
        int nRet;
        do
        {
            MV_CC_DEVICE_INFO_LIST stDeviceList;
            memset(&stDeviceList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));
            //枚举设备
            nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &stDeviceList);
            if (MV_OK != nRet)
            {
                log_error("海康sdk初始化失败,错误码:", nRet);
                break;
            }
            if (stDeviceList.nDeviceNum > 0)
            {
                log_info("检测到的设备");
                for (int i = 0; i < stDeviceList.nDeviceNum; i++)
                {
                    log_info(i);
                    pDeviceInfo = stDeviceList.pDeviceInfo[i];
                    if (nullptr == pDeviceInfo)
                    {
                        break;
                    }
                    PrintDeviceInfo(pDeviceInfo);
                }
            }
            else
            {
                log_info("没有检测到设备");
                break;
            }
            log_info("默认索引设备0");
            unsigned int nIndex = 0;
            if (nIndex >= stDeviceList.nDeviceNum)
            {
                log_error("Intput error!\n");
                break;
            }
            //选择设备并创建句柄
            nRet = MV_CC_CreateHandle(&handle, stDeviceList.pDeviceInfo[nIndex]);
            if (MV_OK != nRet)
            {
                log_error("MV_CC_CreateHandle失败,错误码:", nRet);
                break;
            }
            //打开设备
            nRet = MV_CC_OpenDevice(handle);
            if (MV_OK != nRet)
            {
                log_error("MV_CC_OpenDevice失败,错误码:", nRet);
                break;
            }
            //设置触发模式为off
            //这个实际意义为让相机自行处理图像的采集，不通过外部信号来控制
            //该项目在图像硬同步时需开启
            nRet = MV_CC_SetEnumValue(handle, "TriggerMode", 0);
            if (MV_OK != nRet)
            {
                log_error("MV_CC_SetTriggerMode失败,错误码:", nRet);
                break;
            }
            // ch：设置曝光时间，图像的长宽,和所取图像的偏移
            //注意，这里对offset的值应当提前归零，防止出现长度溢出问题
            nRet = MV_CC_SetIntValue(handle, "OffsetX", 0);
            if (MV_OK != nRet)
            {
                log_error("设置OffsetX错误,错误码:", nRet);
                break;
            }
            nRet = MV_CC_SetIntValue(handle, "OffsetY", 0);
            if (MV_OK != nRet)
            {
                log_error("设置OffsetX错误,错误码:", nRet);
                break;
            }
            nRet = MV_CC_SetFloatValue(handle, "ExposureTime", m_camera.exposure_time);
            if (MV_OK != nRet)
            {
                log_error("设置曝光错误,错误码:", nRet);
                break;
            }
            nRet = MV_CC_SetIntValue(handle, "Width", m_camera.width);
            if (MV_OK != nRet)
            {
                log_error("设置Width错误,错误码:", nRet);
                break;
            }
            nRet = MV_CC_SetIntValue(handle, "Height", m_camera.height);
            if (MV_OK != nRet)
            {
                log_error("设置Height错误,错误码:", nRet);
                break;
            }
            nRet = MV_CC_SetIntValue(handle, "OffsetX", m_camera.offsetX);
            if (MV_OK != nRet)
            {
                log_error("设置Height错误,错误码:", nRet);
                break;
            }
            nRet = MV_CC_SetIntValue(handle, "OffsetY", m_camera.offsetY);
            if (MV_OK != nRet)
            {
                log_error("设置Height错误,错误码:", nRet);
                break;
            }
            // RGB格式0x02180014
            // bayerRG格式0x01080009
            nRet = MV_CC_SetEnumValue(handle, "PixelFormat", 0x01080009);
            if (MV_OK != nRet)
            {
                log_error("设置传输图像格式错误,错误码:", nRet);
                break;
            }
            nRet = MV_CC_SetEnumValue(handle, "TestPattern", 0);
            if (MV_OK != nRet)
            {
                log_error("设置设置关闭测试模式错误,错误码:", nRet);
                break;
            }
            nRet = MV_CC_SetBoolValue(handle, "FrameSpecInfo", 0);
            if (MV_OK != nRet)
            {
                log_error("设置关闭帧水印错误,错误码:", nRet);
                break;
            }
            nRet = MV_CC_SetEnumValue(handle, "AcquisitionMode", 2);
            if (MV_OK != nRet)
            {
                log_error("设置设备采集模式错误,错误码:", nRet);
                break;
            }
            nRet = MV_CC_SetFloatValue(handle, "AcquisitionFrameRate", 999.0);
            if (MV_OK != nRet)
            {
                log_error("设置获取帧率最大值错误,错误码:", nRet);
                break;
            }
            nRet = MV_CC_SetBoolValue(handle, "AcquisitionFrameRateEnable", 1);
            if (MV_OK != nRet)
            {
                log_error("启用对摄像机帧速率的手动控制错误,错误码:", nRet);
                break;
            }
            nRet = MV_CC_SetEnumValue(handle, "ExposureMode", 0);
            if (MV_OK != nRet)
            {
                log_error("设置曝光模式错误,错误码:", nRet);
                break;
            }
            nRet = MV_CC_SetEnumValue(handle, "ExposureAuto", 0);
            if (MV_OK != nRet)
            {
                log_error("关闭自动曝光错误,错误码:", nRet);
                break;
            }
            nRet = MV_CC_SetBoolValue(handle, "HDREnable", 0);
            if (MV_OK != nRet)
            {
                log_error("关闭HDR轮询错误,错误码:", nRet);
                break;
            }
            nRet = MV_CC_SetFloatValue(handle, "Gain", 0.0);
            if (MV_OK != nRet)
            {
                log_error("设置增益错误,错误码:", nRet);
                break;
            }
            nRet = MV_CC_SetEnumValue(handle, "GainAuto", 0);
            if (MV_OK != nRet)
            {
                log_error("关闭自动增益错误,错误码:", nRet);
                break;
            }
            nRet = MV_CC_SetBoolValue(handle, "BlackLevelEnable", 1);
            if (MV_OK != nRet)
            {
                log_error("开启黑电平调整错误,错误码:", nRet);
                break;
            }
            nRet = MV_CC_SetIntValue(handle, "BlackLevel", 100);
            if (MV_OK != nRet)
            {
                log_error("开启黑电平调整错误,错误码:", nRet);
                break;
            }
            nRet = MV_CC_SetEnumValue(handle, "BalanceWhiteAuto", 2);
            if (MV_OK != nRet)
            {
                log_error("关闭连续自动白平衡错误,错误码:", nRet);
                break;
            }
            nRet = MV_CC_SetBoolValue(handle, "AutoFunctionAOIUsageIntensity", 0);
            if (MV_OK != nRet)
            {
                log_error("关闭自动AOI错误,错误码:", nRet);
                break;
            }

            //开始取流
            nRet = MV_CC_StartGrabbing(handle);
            if (MV_OK != nRet)
            {
                log_error("MV_CC_StartGrabbing失败,错误码:", nRet);
                break;
            }
            //获取数据包大小
            MVCC_INTVALUE stParam;
            memset(&stParam, 0, sizeof(MVCC_INTVALUE));
            nRet = MV_CC_GetIntValue(handle, "PayloadSize", &stParam);
            if (MV_OK != nRet)
            {
                log_error("获取数据包大小失败,错误码:", nRet);
                break;
            }

            memset(&stImageInfo, 0, sizeof(MV_FRAME_OUT_INFO_EX));
            pData = (unsigned char *)malloc(sizeof(unsigned char) * stParam.nCurValue);
            if (nullptr == pData)
            {
                break;
            }
            nDataSize = stParam.nCurValue;
        } while (0);
        //如果不正常初始化需要释放资源
        if (nRet != MV_OK)
        {
            if (handle != nullptr)
            {
                MV_CC_DestroyHandle(handle);
                handle = nullptr;
            }
            log_error("相机开启失败");
            throw std::logic_error("相机开启失败");
        }
    }
    open_label = true;
}

cv::Mat &GetFrame::GetOneFrame()
{
    if (open_label)
    {
        if (video_debug_set == 1 || video_debug_set == 2)
        {
            auto ret = capture.read(frame);
            if (ret == false)
            {
                log_error("没有对应的视频");
            }
        }
        else
        {
            auto nRet = MV_CC_GetOneFrameTimeout(handle, pData, nDataSize, &stImageInfo, 1000);
            if (nRet == MV_OK)
            {
                Convert2Mat();
            }
            else
            {
                log_error("没有数据,错误码:", nRet);
                EndCamera();
                throw std::logic_error("相机获取数据失败，如果这是关闭相机时发生请无视，这是关闭相机时获取数据引起的");
            }
        }
    }
    else
    {
        log_error("错误,未开启相机,返回值为默认图像");
    }
#ifdef SAVE_VIDEO
    auto time = std::chrono::high_resolution_clock::now();
    if((int)((time - start_time).count()/TIME_TRANSFORMER) > m_camera.max_video_time)
    {
        video_vapture.release();
    }
    else
    {
        video_vapture.write(frame);
    }
#endif
    return frame;
}

void GetFrame::Convert2Mat()
{
    if (nullptr == pData)
    {
        log_error("没有帧数据");
        return;
    }
    else
    {
        if (PixelType_Gvsp_Mono8 == stImageInfo.enPixelType) // Mono8类型
        {
            frame = cv::Mat(stImageInfo.nHeight, stImageInfo.nWidth, CV_8UC1, pData);
        }
        else if (PixelType_Gvsp_RGB8_Packed == stImageInfo.enPixelType) // RGB8类型
        {
            // Mat像素排列格式为BGR，需要转换
            frame = cv::Mat(stImageInfo.nHeight, stImageInfo.nWidth, CV_8UC3, pData);
            cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);
        }
        else if (PixelType_Gvsp_BayerRG8 == stImageInfo.enPixelType) // BayerRG8类型
        {
            //先转换成opencv的格式
            frame = cv::Mat(stImageInfo.nHeight, stImageInfo.nWidth, CV_8UC1, pData);
            cv::cvtColor(frame, frame, cv::COLOR_BayerRG2RGB);
        }
        else
        {
            log_error("抱歉,不支持该图像格式");
        }
    }
    if (nullptr == frame.data)
    {
        log_error("图像创建失败");
    }
}

bool GetFrame::PrintDeviceInfo(MV_CC_DEVICE_INFO *pstMVDevInfo)
{
    if (nullptr == pstMVDevInfo)
    {
        log_info("pstMVDevInfo指针没有指向任何位置");
        return false;
    }
    if (pstMVDevInfo->nTLayerType == MV_GIGE_DEVICE)
    {
        int nIp1 = ((pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0xff000000) >> 24);
        int nIp2 = ((pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x00ff0000) >> 16);
        int nIp3 = ((pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x0000ff00) >> 8);
        int nIp4 = (pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x000000ff);

        log_info("设备名称为: ", pstMVDevInfo->SpecialInfo.stGigEInfo.chModelName);
        log_info("用户定义的名称为: ", pstMVDevInfo->SpecialInfo.stGigEInfo.chUserDefinedName);
        log_info("暂时不支持IP相机");
    }
    else if (pstMVDevInfo->nTLayerType == MV_USB_DEVICE)
    {
        log_info("设备名称为: ", pstMVDevInfo->SpecialInfo.stUsb3VInfo.chModelName);
        log_info("用户定义的名称为: ", pstMVDevInfo->SpecialInfo.stUsb3VInfo.chUserDefinedName);
    }
    else
    {
        log_info("不支持的设备.\n");
    }
    return true;
}

void GetFrame::EndCamera()
{
    open_label = false;
#ifdef SAVE_VIDEO
    video_vapture.release();
#endif
    if (video_debug_set == 1 || video_debug_set == 2)
    {
        capture.release();
    }
    else if (video_debug_set == 3)
    {
        return;
    }
    else
    {
        int nRet;
        do
        {
            //释放数据指针
            free(pData);
            pData = nullptr;
            //关闭设备
            nRet = MV_CC_CloseDevice(handle);
            if (MV_OK != nRet)
            {
                log_error("MV_CC_CloseDevice失败,错误码:", nRet);
                log_error("如果相机已正常关闭，请忽略，这是重复关闭相机引起的");
                break;
            }
            //销毁句柄
            nRet = MV_CC_DestroyHandle(handle);
            if (MV_OK != nRet)
            {
                log_error("MV_CC_DestroyHandle失败,错误码:", nRet);
                log_error("如果相机已正常关闭，请忽略，这是重复关闭相机引起的");
                break;
            }
        } while (0);
        //如果不正常退出需要释放资源
        if (nRet != MV_OK)
        {
            if (handle != nullptr)
            {
                MV_CC_DestroyHandle(handle);
                handle = nullptr;
            }
        }
    }

}