// 封装sockfd
#pragma once

#include "noncopyable.h"

class InetAddress;

class Socket : noncopyable
{
public:
    explicit Socket(int sockfd) : sockfd_(sockfd) {}
    ~Socket();

    int fd() const { return sockfd_; }
    void bindAddress(const InetAddress &localaddr); // 调用bind()绑定服务器IP端口
    void listen(); // 调用listen()，监听acceptSock
    int accept(InetAddress *peeraddr); // 调用accept4()

    void shutdownWrite(); // 关闭写端口

    // 调用setsockopt()设置标志位
    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on); // 启动TCP Sock的保活机制
private:
    const int sockfd_;
};
