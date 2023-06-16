#ifndef TOE_SERIAL_H_
#define TOE_SERIAL_H_

#include <vector>
#include "debug.hpp"
#include "toe_json.hpp"
#include "asio.hpp"

namespace toe
{
    class toe_serial
    {
    public:
        toe_serial();
        ~toe_serial();
        //返回封装好的模式和颜色信息
        bool get_msg(int & color, int & mode);
        //发送封装好的坐标信息
        bool send_msg(const std::vector<double> & msg);
    private:
        // 轮询打开串口
        void init_port();
        // boost串口相关    
        asio::io_service io_s;
        asio::serial_port serial_fd = asio::serial_port(io_s);
        // 接收发送缓冲区
        uint8_t rbuff[1024];
        uint8_t sbuff[29];
        //记录的模式信息
        int mode = DEFALUTE_MODE;
        //记录的颜色信息
        bool color = DEFALUTE_COLOR;
    };
    
}

#endif