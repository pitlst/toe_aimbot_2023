#pragma once
#include <boost/asio.hpp>

#include <vector>
#include <fstream>

#include "debug.h"

namespace toe
{
    // 数据转换用联合体
    union acm_data{
        uint8_t     bit[4];
        float       data;
    };

    class Serial final
    {
    public:
        Serial();
        ~Serial() = default;
        //返回封装好的模式和颜色信息
        std::vector<float> get_msg();
        //发送封装好的坐标信息
        void send_msg(const std::vector<double> &msg);

    private:
        // 轮询打开串口
        void init_port();
        // boost串口相关    
        boost::asio::io_service io_s;
        boost::asio::serial_port serial_fd = boost::asio::serial_port(io_s);
        boost::system::error_code m_ec;
        // 接收发送缓冲区
        uint8_t rbuff[1024];
        uint8_t sbuff[29];
        //记录的模式信息
        int mode = DEFALUTE_MODE;
        //记录的颜色信息
        bool color = DEFALUTE_COLOR;
        //用于记录串口文件的文件流
        std::fstream m_file;
    };
}