// 运行在mainLoop，监听acceptSocket，执行listen和accept
#pragma once

#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"

#include <functional>

class EventLoop;
class InetAddress;

class Acceptor : noncopyable
{
public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress&)>;

    Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport);
    ~Acceptor();

    // TcpServer调用，设置TcpServer::newConnection
    void setNewConnectionCallback(const NewConnectionCallback &cb)
    {
        newConnectionCallback_ = cb;
    }

    bool listenning() const { return listenning_; }
    void listen();

private:
    void handleRead(); // mainLoop监听到可读事件后的事件处理，用于处理新用户连接，类似accept()
    
    EventLoop *loop_; // Acceptor使用baseLoop即mainLoop，acceptSocket由mainLoop循环监听
    Socket acceptSocket_;
    Channel acceptChannel_; // 连接后由poller监听事件
    NewConnectionCallback newConnectionCallback_; // 把connd包装为channel发给subLoop
    bool listenning_;
};
