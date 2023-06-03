//本头文件用于定义常用宏,方便功能测试
//注意，更改本文件后需要重新编译后运行
#pragma once
#include <cmath>
#include <string>
#define _USE_MATH_DEFINES 

//---------------------------------路径参数，注意需要针对计算机更改绝对路径---------------------------------//
//参数文件路径
#define PATH_ASSET std::string("/home/cheakf/Documents/toe_aimbot_2023/asset/")
//默认串口类型
#define PATH_SERIAL "ACM"
//相机输入源路径
// #define PATH_CAPTURE "asset/2.avi"

//-----------------------------------------功能编译开关-----------------------------------------//

//开启此宏定义关闭所有日志输出
// #define NO_LOG

//开启此宏定义关闭日志文件写入
#define NO_LOG_FILE

//开启此宏定义关闭串口,使用默认数据
#define SERIAL_CLOSE

//此宏定义决定log等级,debug下会默认开启调试信息输出,正常比赛请开启release
//注意，该项只影响log，具体的编译优化在cmakelist中
// #define LOG_RELEASE
#define  LOG_DEBUG



//---------------------------------默认参数，一般不更改---------------------------------//
//默认的识别颜色
#define DEFALUTE_COLOR 1
//默认的识别模式
#define DEFALUTE_MODE 2
//debug下灯条的默认颜色
#define LIGHTBAR_COLOR (0,255,0)
//debug下装甲板的默认颜色
#define ARMOR_COLOR (0,255,255)
//debug下文字的默认颜色
#define TEXT_COLOR (255,255,255)
//debug下图像的默认线宽
#define FRAME_THICKNESS 2
//Tracker做相似度筛选时默认不识别的最大值
#define TRACK_SCORE_MAX 999999.0


//---------------------------------宏定义处理，一般不更改---------------------------------//
//chono系统时钟调用单位转换常数
#define TIME_TRANSFORMER 1000000000.0
//如果什么都没定义默认release
#ifndef LOG_RELEASE
#ifndef LOG_DEBUG
#define LOG_RELEASE
#endif
#endif
//如果定义了release就关闭debug的定义
#ifdef LOG_RELEASE
#undef LOG_DEBUG
#endif