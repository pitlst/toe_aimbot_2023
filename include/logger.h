#pragma once
#ifndef SWQ_LOGGER
#define SWQ_LOGGER

#include <sstream>
#include <fstream>
#include <vector>
#include <deque>
#include <list>
#include <string>
#include <ctime>
#include <initializer_list>
#include <iostream>
#include <chrono>

#include "debug.h"

// 定义一下日志文件所在地

#define log_debug(...) \
    toe::logger::instance().log(toe::logger::DEBUG, toe::logger::instance().format(__VA_ARGS__))

#define log_info(...) \
    toe::logger::instance().log(toe::logger::INFO, toe::logger::instance().format(__VA_ARGS__))

#define log_warn(...) \
    toe::logger::instance().log(toe::logger::WARN, toe::logger::instance().format(__VA_ARGS__))

#define log_error(...) \
    toe::logger::instance().log(toe::logger::ERROR, toe::logger::instance().format(__VA_ARGS__))

#define log_fatal(...) \
    toe::logger::instance().log(toe::logger::FATAL, toe::logger::instance().format(__VA_ARGS__))

namespace toe
{

    class logger final
    {
    public:
        enum Level
        {
            DEBUG,
            INFO,
            WARN,
            ERROR,
            FATAL,
            NONE
        };

        std::vector<std::string> label =
            {
                "DEBUG",
                "INFO",
                "WARN",
                "ERROR",
                "FATAL",
        };

        static logger &instance();
        void set_level(Level input_level);
        void open(const std::string &filename);
        void log(Level level, const std::string &input);
        void close();

        // 模板推导，直接在头文件实现，格式化输入的变量
        template <typename T, typename... args>
        std::string format(const T &input, const args &...rest)
        {
            ss << input;
            return format(rest...);
        }
        // 包中最后一个元素
        template <typename T>
        std::string format(const T &input)
        {
            ss << input;
            auto temp = ss.str();
            // 清空保存的字符串
            ss.str(std::string());
            // 字符串流复位
            ss.clear();
            return temp;
        }
        std::string format()
        {
            return "\n";
        }

    private:
        logger();
        logger(const std::string &filepath);
        ~logger();

        //用于标志是否写入文件
#ifdef NO_LOG_FILE
        int m_flabel = 0;
#endif
#ifndef NO_LOG_FILE
        int m_flabel = 1;
#endif
        Level m_level;
        std::string m_filename;
        std::fstream m_file;
        std::stringstream ss;
    };

    inline logger::logger()
    {
#ifdef NO_LOG
        m_level = NONE;
#else
#ifdef LOG_DEBUG
        m_level = DEBUG;
#endif
#ifdef LOG_RELEASE
        m_level = INFO;
#endif
#ifdef LOG_MINSIZEREL
        m_level = FATAL;
#endif
#endif
        open("");
    }

    inline logger::logger(const std::string &filepath)
    {
#ifdef NO_LOG
        m_level = NONE;
#else
#ifdef LOG_DEBUG
        m_level = DEBUG;
#endif
#ifdef LOG_RELEASE
        m_level = INFO;
#endif
#ifdef LOG_MINSIZEREL
        m_level = FATAL;
#endif
#endif
        open(filepath);
    }

    inline logger::~logger()
    {
        close();
    }

    inline logger &logger::instance()
    {
        static logger m_instance(PATH_ASSET + "log.txt");
        return m_instance;
    }

    inline void logger::set_level(Level input_level)
    {
#ifdef NO_LOG
        m_level = NONE;
#else
        m_level = input_level;
#endif
    }

    inline void logger::open(const std::string &filename)
    {
        m_filename = filename;
        if (m_filename.empty())
        {
            return;
        }
        // 以追加模式打开文件
        m_file.open(filename, std::ios::app);
        if (m_file.fail())
        {
            throw std::logic_error("open log file failed: " + filename);
        }
        // 将文件指针定位到文件的末尾
        m_file.seekg(0, std::ios::end);
        // 获取系统时间
        time_t rawtime = std::chrono::system_clock::to_time_t(std::chrono::high_resolution_clock::now());
        struct tm *ptminfo = localtime(&rawtime);
        std::stringstream ss;
        ss << ptminfo->tm_year + 1900;
        ss << "-";
        ss << ptminfo->tm_mon + 1;
        ss << "-";
        ss << ptminfo->tm_mday;
        ss << " ";
        ss << ptminfo->tm_hour;
        ss << ":";
        ss << ptminfo->tm_min;
        ss << ":";
        ss << ptminfo->tm_sec;
        ss << ":";
        ss << label[m_level];
        std::cout << "----------" << ss.str() << "----------" << std::endl;
        // 在日志中表示开始下一次写入文件
        if(m_flabel)
        {
            m_file << "----------" << ss.str() << "----------" << std::endl;
        }
    }

    inline void logger::log(Level level, const std::string &input)
    {
        if (m_level > level)
        {
            return;
        }
        if (m_filename.empty() && !m_file.good())
        {
            throw std::logic_error("open log file failed: " + m_filename);
        }
        // 获取系统时间
        time_t rawtime = std::chrono::system_clock::to_time_t(std::chrono::high_resolution_clock::now());
        struct tm *ptminfo = localtime(&rawtime);
        std::stringstream m_ss;
        // 写入系统时间
        m_ss << ptminfo->tm_year + 1900;
        m_ss << "-";
        m_ss << ptminfo->tm_mon + 1;
        m_ss << "-";
        m_ss << ptminfo->tm_mday;
        m_ss << " ";
        m_ss << ptminfo->tm_hour;
        m_ss << ":";
        m_ss << ptminfo->tm_min;
        m_ss << ":";
        m_ss << ptminfo->tm_sec;
        m_ss << ": ";
        m_ss << label[level];
        m_ss << " :: ";
        m_ss << input;
        // 写入命令行
        std::cout << m_ss.str() << std::endl;
        // 写入文件
        if (!m_filename.empty() && m_flabel)
        {
            m_file << m_ss.str() << std::endl;
        }
    }

    inline void logger::close()
    {
        if (m_filename.empty())
        {
            return;
        }
        else
        {
            m_file.close();
        }
    }
}

#endif