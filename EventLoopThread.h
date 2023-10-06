#pragma once

#include "noncopyable.h"
#include "Thread.h"

#include <functional>
#include <mutex>
#include <condition_variable>
#include <string>

class EventLoop;


class EventLoopThread : noncopyable
{
public:
    // EventLoop初始化线程
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(),
        const std::string &name = std::string());
    ~EventLoopThread();

    // 开启一个loop和线程，返回loop
    EventLoop* startLoop();
private:
    void threadFunc();

    EventLoop *loop_;
    bool exiting_;
    Thread thread_; // 绑定当前loop_的线程
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallback callback_;
};
