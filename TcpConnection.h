#pragma once

#include "noncopyable.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "Timestamp.h"

#include <memory>
#include <string>
#include <atomic>

class Channel;
class EventLoop;
class Socket;

class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection> // 使用shared_from_this
{
public:
    TcpConnection(EventLoop *loop,
                const std::string &nameArg,
                int sockfd,
                const InetAddress& localAddr,
                const InetAddress& peerAddr);
    ~TcpConnection();

    EventLoop* getLoop() const { return loop_; }
    const std::string& name() const { return name_; }
    const InetAddress& localAddress() const { return localAddr_; }
    const InetAddress& peerAddress() const { return peerAddr_; }

    bool connected() const { return state_ == kConnected; }

    // 发送数据，默认为string
    void send(const std::string &buf);
    // 关闭连接
    void shutdown();

    void setConnectionCallback(const ConnectionCallback &cb)
    { connectionCallback_ = cb; }

    void setMessageCallback(const MessageCallback &cb)
    { messageCallback_ = cb; }

    void setWriteCompleteCallback(const WriteCompleteCallback &cb)
    { writeCompleteCallback_ = cb; }

    void setHighWaterMarkCallback(const HighWaterMarkCallback &cb, size_t highWaterMark)
    { highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark; }

    void setCloseCallback(const CloseCallback &cb)
    { closeCallback_ = cb; }

    // 连接建立
    void connectEstablished();
    // 连接销毁
    void connectDestroyed();

private:
    // socket状态
    enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };
    void setState(StateE state) { state_ = state; } // 不能设置public，因为参数类型私有

    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    // 发送数据：应用发送快，内核处理慢，所以置缓冲
    void sendInLoop(const void *message, size_t len);
    void shutdownInLoop();
    
    EventLoop *loop_; // 绝对不是baseLoop_，因为TcpConnection是在里面subLoop管理的
    const std::string name_;
    std::atomic_int state_;
    bool reading_;

    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    /**
     * 通知路线：用户->TcpServer->Acceptor->TcpConnection设置回调
     * ->Channel->poller->Channel的回调操作
    */

    ConnectionCallback connectionCallback_; // 新连接或者连接断开时调用
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;
    CloseCallback closeCallback_;
    size_t highWaterMark_; // 高水位就是控制双方数据的收发要保持限度内

    Buffer inputBuffer_; // 接收数据的缓冲
    Buffer outputBuffer_; // 发送数据的缓冲
};
