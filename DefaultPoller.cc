// Poller的newDefaultPoller()实现
#include "Poller.h"
#include "EPollPoller.h"

#include <stdlib.h> // getenv

Poller* Poller::newDefaultPoller(EventLoop *loop)
{
    if (::getenv("MUDUO_USE_POLL")) // 一般没有设置该环境变量，所以都是epoll
    {
        return nullptr;
    }
    else
    {
        return new EPollPoller(loop); // 生成epoll实例
    }
}