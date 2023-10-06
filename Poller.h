// 多路事件分发器的抽象
#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <vector>
#include <unordered_map>

class Channel;
class EventLoop;

class Poller : noncopyable
{
public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop *loop);
    virtual ~Poller() = default;

    // EventLoop调用poll，poll通过epoll_wait把发生事件填写到activeChannels
    virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;
    // 在poller中更新channel
    virtual void updateChannel(Channel *channel) = 0;
    // 从poller中删除channel
    virtual void removeChannel(Channel *channel) = 0;

    // 判断poller是否有目标channel
    bool hasChannel(Channel *channel) const;

    // 用于Eventloop获取默认IO复用的具体实现（对象）
    static Poller* newDefaultPoller(EventLoop *loop);
protected:
    // channels包含多个channel key:sockfd, value:channel
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channels_;
private:
    EventLoop *ownerLoop_;
};