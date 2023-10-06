// poller的epoll实现
#pragma once

#include "Poller.h"
#include "Timestamp.h"

#include <vector>
#include <sys/epoll.h>

/**
 * epoll包括以下方法
 * epoll_create：epollfd_
 * epoll_ctl：updateChannel、removeChannel
 * epoll_wait：poll方法
*/

class EPollPoller : public Poller
{
public:
    EPollPoller(EventLoop *loop);
    ~EPollPoller() override; // 覆盖函数，基类的方法是虚函数

    // EventLoop调用poll，poll通过epoll_wait把发生事件填写到activeChannels
    Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
    // 在poller（epoll队列）中更新channel
    void updateChannel(Channel *channel) override;
    // 从poller（epoll队列）中删除channel 
    void removeChannel(Channel *channel) override; 

private:
    static const int kInitEventListSize = 16;

    // poll调用，填写活跃连接到activeChannels
    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
    // updateChannel调用，类似event_ctl
    void update(int operation, Channel *channel);

    using EventList = std::vector<epoll_event>;

    // epoll_create的epollfd
    int epollfd_;
    // 事件列表
    EventList events_;
};
