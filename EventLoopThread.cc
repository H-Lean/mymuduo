#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb,
		const std::string &name)
		: loop_(nullptr)
		, exiting_(false)
		, thread_(std::bind(&EventLoopThread::threadFunc, this), name)
		, mutex_()
		, cond_()
		, callback_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != nullptr)
    {
        loop_->quit();  // 退出线程绑定的事件循环
        thread_.join(); // 等待thread_结束
    }
}

EventLoop *EventLoopThread::startLoop()
{
    thread_.start(); // 启动底层的新线程

    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (loop_ == nullptr) // wait被唤醒后要保证loop_满足条件，不然继续wait
        {
            cond_.wait(lock);
        }
        loop = loop_; // 等待threadFunc通知后才获取loop_
    }
    return loop;
}

void EventLoopThread::threadFunc()
{
    EventLoop loop; // 创建一个eventLoop，和上面的线程一一对应，one loop per thread

    if (callback_)
    {
        callback_(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }

    loop.loop(); // 开启事件循环，监听poller
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr; // 服务器程序关闭
}
