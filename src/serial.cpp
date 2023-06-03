//linux系统的头文件
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <errno.h>
#include <memory.h>

#include "logger.h"
#include "serial.h"


using namespace toe;

Serial::Serial()
{
    init_port();
}

void Serial::init_port()
{
#ifndef SERIAL_CLOSE
    std::string post_path;
    char post_num = 0;
    // 目前只做了USB虚拟串口和真串口的兼容
    if (PATH_SERIAL == "ACM")
    {
        post_path = "/dev/ttyACM0";
    }
    else
    {
        post_path = "/dev/ttyUSB0";
    }
    
    // 轮询打开串口
    while (1)
    {
        try
        {
            serial_fd.open(post_path, m_ec);
            break;
        }  
        catch(std::exception& err)
        {
            log_debug("串口打开失败");
            post_num++;
            post_path[post_path.size()-1] = (post_num + '0');
            // 只遍历0到10号串口
            if (post_num >= 9)
            {
                post_num = 0;
            }
            continue;
        }
    }
    //设置串口参数  
    serial_fd.set_option(boost::asio::serial_port::baud_rate(115200), m_ec);
    if (m_ec)
    {
        log_error("串口参数设置失败，错误信息:", m_ec.message());
    }
    serial_fd.set_option(boost::asio::serial_port::flow_control(boost::asio::serial_port::flow_control::none), m_ec);  
    if (m_ec)
    {
        log_error("串口参数设置失败，错误信息:", m_ec.message());
    }
    serial_fd.set_option(boost::asio::serial_port::parity(boost::asio::serial_port::parity::none), m_ec);  
    if (m_ec)
    {
        log_error("串口参数设置失败，错误信息:", m_ec.message());
    }
    serial_fd.set_option(boost::asio::serial_port::stop_bits(boost::asio::serial_port::stop_bits::one), m_ec);  
    if (m_ec)
    {
        log_error("串口参数设置失败，错误信息:", m_ec.message());
    }
    serial_fd.set_option(boost::asio::serial_port::character_size(8), m_ec);
    if (m_ec)
    {
        log_error("串口参数设置失败，错误信息:", m_ec.message());
    }
#endif
}

std::vector<float> Serial::get_msg()
{
#ifndef SERIAL_CLOSE
    memset(&rbuff,0,sizeof(rbuff));
    serial_fd.read_some(boost::asio::buffer(rbuff), m_ec);  
    float yaw = -1;
    float pit = -1; 
    float roll = -1;
    if (!m_ec)
    {
        // 校验帧头
        if (int(rbuff[0]) == 166)
        {
            size_t start_index = 1;
            // color1为蓝色,0为红色
            if (int(rbuff[start_index++]) > 100)
            {
                color = true;
            }
            else
            {
                color = false;
            }
            // 模式信息:0为自瞄,1为参数整定
            mode = int(rbuff[start_index++]);
            // 跳过crc校验位
            start_index++;
            acm_data temp;
            temp.bit[0] = rbuff[start_index++];
            temp.bit[1] = rbuff[start_index++];
            temp.bit[2] = rbuff[start_index++];
            temp.bit[3] = rbuff[start_index++];
            yaw = temp.data;
            temp.bit[0] = rbuff[start_index++];
            temp.bit[1] = rbuff[start_index++];
            temp.bit[2] = rbuff[start_index++];
            temp.bit[3] = rbuff[start_index++];
            pit = temp.data;
            temp.bit[0] = rbuff[start_index++];
            temp.bit[1] = rbuff[start_index++];
            temp.bit[2] = rbuff[start_index++];
            temp.bit[3] = rbuff[start_index++];
            roll = temp.data;
        }
        else
        {
            log_error("未知的帧头 ", int(rbuff[0]));
        }
    }
    else
    {
        log_error("串口读取失败，错误信息:", m_ec.message()); 
        if (serial_fd.is_open() == false)
        {
            init_port();
        }
    }
    std::vector<float> temp_return = {(float)color, (float)mode, yaw, pit, roll};
    return temp_return;
#endif
#ifdef SERIAL_CLOSE
    std::vector<float> temp_return = {(float)color, (float)mode, -1, -1, -1};
    return temp_return;
#endif
}

void Serial::send_msg(const std::vector<double> &msg)
{
#ifndef SERIAL_CLOSE
    if (msg.size() >= 6)
    {
        acm_data temp;
        temp.data = (float)msg[0];
        sbuff[0] = temp.bit[0];
        sbuff[1] = temp.bit[1];
        sbuff[2] = temp.bit[2];
        sbuff[3] = temp.bit[3];
        temp.data = (float)msg[1];
        sbuff[4] = temp.bit[0];
        sbuff[5] = temp.bit[1];
        sbuff[6] = temp.bit[2];
        sbuff[7] = temp.bit[3];
        temp.data = (float)msg[2];
        sbuff[8] = temp.bit[0];
        sbuff[9] = temp.bit[1];
        sbuff[10] = temp.bit[2];
        sbuff[11] = temp.bit[3];
        temp.data = (float)msg[3];
        sbuff[12] = temp.bit[0];
        sbuff[13] = temp.bit[1];
        sbuff[14] = temp.bit[2];
        sbuff[15] = temp.bit[3];
        temp.data = (float)msg[4];
        sbuff[16] = temp.bit[0];
        sbuff[17] = temp.bit[1];
        sbuff[18] = temp.bit[2];
        sbuff[19] = temp.bit[3];
        temp.data = (float)msg[5];
        sbuff[20] = temp.bit[0];
        sbuff[21] = temp.bit[1];
        sbuff[22] = temp.bit[2];
        sbuff[23] = temp.bit[3];
        temp.data = (float)msg[6];
        sbuff[24] = temp.bit[0];
        sbuff[25] = temp.bit[1];
        sbuff[26] = temp.bit[2];
        sbuff[27] = temp.bit[3];
        sbuff[28] = (uint8_t)msg[7];

        serial_fd.write_some(boost::asio::buffer(sbuff) ,m_ec);  
        if(m_ec)  
        {  
            log_error("串口写入失败，错误信息:", m_ec.message());  
            if (serial_fd.is_open() == false)
            {
                init_port();
            }
        }
    }
    else
    {
        log_error("发送信息长度错误");
    }
#endif
}