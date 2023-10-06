#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"

#include <errno.h>
#include <unistd.h> // close
#include <strings.h> // memset

// channel未添加到poller和channels_
const int kNew = -1; // channel的成员index_ = -1
// channel已添加到poller中
const int kAdded = 1;
// channel曾在poller中，被删除了，但还在channels_
const int kDeleted = 2;

EPollPoller::EPollPoller(EventLoop *loop)
    : Poller(loop)
	, epollfd_(::epoll_create1(EPOLL_CLOEXEC))
	, events_(kInitEventListSize) // vector<epoll_event>
{
    if (epollfd_ < 0)
    {
        LOG_FATAL("epoll_create error:%d \n", errno);
    }
}

EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    // 实际上应该用LOG_DEBUG输出日志更合理
    LOG_INFO("func=%s => fd total count:%lu \n", __FUNCTION__, channels_.size());
    // &*events_.begin()返回数组首地址，epoll_wait监听clientfd和wakeupfd
    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    int saveErrno = errno;
    Timestamp now(Timestamp::now());

    if (numEvents > 0)
    {
        LOG_INFO("%d events happened \n", numEvents);
        fillActiveChannels(numEvents, activeChannels);
        if (numEvents == events_.size()) // 发生事件太多，需要扩容
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if (numEvents == 0) // 超时 
    {
        LOG_DEBUG("%s timeout! \n", __FUNCTION__);
    }
    else // 发生错误
    {
        if (saveErrno != EINTR)
        {
            errno = saveErrno; // 保证errno没有被改变
            LOG_ERROR("EPollPoller::poll() err! \n");
        }
    }
    return now;
}

void EPollPoller::updateChannel(Channel *channel) 
{
    const int index = channel->index();
    LOG_INFO("func=%s => fd=%d events=%d index=%d \n", __FUNCTION__, channel->fd(), channel->events(), index);
    
    if (index == kNew || index == kDeleted) // 如果是完全没在或者曾经在epoll队列中的，就添加到epoll队列中
    {
        if (index == kNew) // 添加到channels_
        {
			int fd = channel->fd();
			channels_[fd] = channel;
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel); // 添加到epoll队列中
    }
    else // channel在poller上注册过
    {
		int fd = channel->fd();
        if (channel->isNoneEvent()) // 对任何事件都不感兴趣
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else // 已注册但事件可能需要更改
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}


void EPollPoller::removeChannel(Channel *channel) 
{
    int fd = channel->fd();
    channels_.erase(fd);

    LOG_INFO("func=%s => fd=%d\n", __FUNCTION__, fd);

    int index = channel->index();
    if (index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew); // 设置为从来没有在poller中添加过
}

void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
    for (int i = 0; i < numEvents; ++i)
    {
        Channel *channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}
void EPollPoller::update(int operation, Channel *channel)
{
    epoll_event event;
    bzero(&event, sizeof event);
    
    int fd = channel->fd();

    event.events = channel->events();
	event.data.fd = fd;
    event.data.ptr = channel;
    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0) // 操作执行错误
    {
        if (operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl del error: %d\n", errno);
        }
        else
        {
            LOG_FATAL("epoll_ctl add/mod error: %d\n", errno);
        }
    }
}


