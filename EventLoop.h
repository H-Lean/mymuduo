#pragma once

#include <functional>
#include <vector>
#include <atomic>
#include <memory> // unique_ptr
#include <mutex>

#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"

class Channel;
class Poller;

// 事件循环类 主要包含了Channel Poller(epoll的抽象)
class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // 开启事件循环
    void loop();
    // 退出事件循环
    void quit();

    // 返回时间戳
    Timestamp pollReturnTime() const { return pollReturnTime_; }

    // 执行cb并判断是否在当前loop中
    void runInLoop(Functor cb);
    // 把cb放入队列中，唤醒loop相应线程，执行cb
    void queueInLoop(Functor cb);

    // 唤醒loop所在线程
    void wakeup();

    // EventLoop的方法 调用 Poller的方法
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);

    // 判断EventLoop对象是否在自己的线程
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }
private:
    // subLoop执行，通过监听wakeupFd_被唤醒，处理mainReactor发送的新用户channel
    void handleRead();
    // 执行回调
    void doPendingFunctors();

    using ChannelList = std::vector<Channel*>;

    std::atomic_bool looping_; // 原子操作
    std::atomic_bool quit_; // 标识退出loop循环

    const pid_t threadId_; // 当前loop线程的tid
    
    Timestamp pollReturnTime_; // poller返回发生事件的时间
    std::unique_ptr<Poller> poller_;

    // 当mainLoop获取一个新用户的channel，轮询选择一个subloop，用wakeupFd_唤醒以处理channel
    int wakeupFd_; 
    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activeChannels_;

	std::atomic_bool callingPendingFunctors_; // 标识当前loop是否有要执行的回调
	std::vector<Functor> pendingFunctors_; // 存储loop需要执行的所有回调
    std::mutex mutex_; // 互斥锁，保护上面vector容器的线程安全
};
