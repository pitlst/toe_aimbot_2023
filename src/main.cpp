// 系统库
#include <signal.h>
// 标准库
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
// 自定义头文件
#include "get_frame.h"
#include "serial.h"


// 线程退出控制标志位
bool terminal_label = false;

static void my_handler(int sig);

int main(int argc, char **argv)
{
    log_debug("hello ros world");
    // 接收并处理控制台输入
    signal(SIGINT, my_handler);

    toe::GetFrame capture("HIVISION");
    toe::Serial serial;

    cv::Mat frame;
    // waitkey下的按键返回值
    int k = 0;

    // 开启识别
    while (1)
    {
        // 检查控制台输入并退出
        if (terminal_label || k == 27)
        {
            log_debug("正在退出");
            cv::destroyAllWindows();
            break;
        }

        frame = capture.GetOneFrame();
        cv::imshow("frame",frame);
        k = cv::waitKey(1);
    }
    terminal_label = false;
    return 0;
}

// 重载控制台输入强制终止回调函数
static void my_handler(int sig)
{
    log_debug("正在退出");
    terminal_label = true;
}