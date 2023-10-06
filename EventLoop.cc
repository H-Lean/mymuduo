#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h> // eventfd
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>

// 防止一个线程创建多个EventLoop； __thread把变量设为thread_local
__thread EventLoop *t_loopInThisThread = nullptr;

// 默认的poller IO复用接口的超时时间
const int kPollTimeMs = 10000;

// 创建wakeupfd，用来notify唤醒subReactor处理新的channel
int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG_FATAL("eventfd error:%d \n", errno);
    }
    return evtfd;
}

EventLoop::EventLoop()
    : looping_(false)
	, quit_(false)
	, callingPendingFunctors_(false)
	, threadId_(CurrentThread::tid())
	, poller_(Poller::newDefaultPoller(this))
	, wakeupFd_(createEventfd())
	, wakeupChannel_(new Channel(this, wakeupFd_))
{
    LOG_DEBUG("EventLoop created %p in thread %d \n", this, threadId_);
    if (t_loopInThisThread) // 确保one loop per thread
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n", t_loopInThisThread, threadId_);
    }
    else
    {
        t_loopInThisThread = this;
    }

    // 设置wakeupfd_的事件类型和发生事件后的回调操作
    // readcb不是需要传入timestamp吗？handleRead并没有？
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    // 每个eventLoop都会监听wakeupChannel的EPOLLIN读事件
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping \n", this); // 输出地址/指针

    while (!quit_)
    {
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        for (Channel *channel : activeChannels_)
        {
            // EventLoop通知channel处理相应事件
            channel->handleEvent(pollReturnTime_);
        }
        // 执行EventLoop要处理的回调操作
        /**
         * mainLoop用于accept连接，返回fd和打包额外信息的channel，
         * 分发给subLoop做事件处理
         * mainLoop会事先注册cb，在wakeup subLoop后，由subLoop执行cb
         */
        doPendingFunctors();
    }

    LOG_INFO("EventLoop %p stop looping. \n", this);
    looping_ = false; // 结束循环
}

// 退出事件循环
void EventLoop::quit()
{
    quit_ = true; // 在自己线程中调用quit_就结束while

    /**
     * 其他线程调用自己线程，需要先唤醒它，
     * 然后在loop函数中，唤醒的线程就会结束poll调用，
     * 接着由于quit_为false而结束while
     */
    if (!isInLoopThread())
    {
        wakeup();
    }
}

void EventLoop::runInLoop(Functor cb)
{
    if (isInLoopThread())
    {
        cb();
    }
    else
    {
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }

    // callingPendingFunctors_表示当前loop正在执行cb，但有了新cb，为了不让loop函数的poll阻塞，需要唤醒然后继续执行cb
    if (!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup();
    }
}

void EventLoop::handleRead()
{
    uint64_t one = 1;                              // 8个字节
    ssize_t n = read(wakeupFd_, &one, sizeof one); // 读取什么不重要，主要是通知唤醒
    if (n != sizeof one)
    {
        LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8 \n", n);
    }
}

void EventLoop::wakeup()
{
    uint64_t one = 1;
    // 向wakeupFd_写数据，wakeupChannel就会发生读事件，从而在loop函数中唤醒线程
    ssize_t n = write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n", n);
    }
}

void EventLoop::updateChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel)
{
    return poller_->hasChannel(channel);
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_); // 出了作用域，锁就没了
        /**
         * swap原因：在queueInLoop加入cb的时候，没有办法去执行cb，
         * 而swap之后，虽然pendingFunctors_为空，不影响加入cb，
         * 同时也保证cb的及时执行
        */        
        functors.swap(pendingFunctors_);
    }

    for (const Functor &functor : functors)
    {
        functor(); // 执行当前loop需要执行的回调操作
    }

    callingPendingFunctors_ = false; // 结束回调
}
