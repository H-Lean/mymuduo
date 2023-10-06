// 日志文件
#pragma once

#include <string> // 不同的include比如自己的和库里的 做区别

#include "noncopyable.h"

// 定义4种日志级别的宏，用户只需要专注写日志，其余由宏
// 用户使用：LOG_INFO("%s %d", arg1, arg2)
#define LOG_INFO(logmsgFormat, ...) \
    do \
    { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(INFO); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(0) // 使用do while有很多原因，最主要是把它作为一个独立单元，避免与调用处上下文混淆，并且大多数系统都可以识别无效的while

#define LOG_ERROR(logmsgFormat, ...) \
    do \
    { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(ERROR); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(0) 

#define LOG_FATAL(logmsgFormat, ...) \
    do \
    { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(FATAL); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
        exit(-1); \
    } while(0) 

#ifdef MUDEBUG // 由于调试信息太多，影响效率，设置debug开关
#define LOG_DEBUG(logmsgFormat, ...) \
    do \
    { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(DEBUG); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(0) 
#else
    #define LOG_DEBUG(logmsgFormat, ...) // 空
#endif

// 定义日志的级别  INFO  ERROR  FATAL  DEBUG 
enum LogLevel
{
    INFO,  // 普通信息
    ERROR, // 错误信息
    FATAL, // core信息 致命的奔溃错误信息
    DEBUG, // 调试信息
};

// 输出一个日志类
class Logger : noncopyable
{
public:
    // 获取日志唯一的实例对象
    static Logger& instance(); // 返回引用因为使用.成员方便，static唯一
    // 设置日志级别
    void setLogLevel(int level);
    // 写日志
    void log(std::string msg);
private:
    int logLevel_; // 末尾的下划线区分系统变量，同时区分成员函数和成员变量
    // Logger() {};
};
