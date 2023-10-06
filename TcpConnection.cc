#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"

#include <functional>
#include <errno.h>
#include <sys/types.h>         
#include <sys/socket.h>
#include <strings.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <string>

static EventLoop *CheckLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d TcpConnection Loop is null! \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop,
                             const std::string &nameArg,
                             int sockfd,
                             const InetAddress &localAddr,
                             const InetAddress &peerAddr)
    : loop_(CheckLoopNotNull(loop))
	, name_(nameArg)
	, state_(kConnecting)
	, reading_(true)
	, socket_(new Socket(sockfd))
	, channel_(new Channel(loop, sockfd))
	, localAddr_(localAddr)
	, peerAddr_(peerAddr)
	, highWaterMark_(64 * 1024 * 1024) // 64M
{
    // 设置channel的回调，poller给channel通知感兴趣的事件发生，channel就会执行回调
    channel_->setReadCallback(
        std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(
        std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(
        std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(
        std::bind(&TcpConnection::handleError, this));

    LOG_INFO("TcpConnection::ctor[%s] at fd=%d \n", name_.c_str(), sockfd);
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d \n",
             name_.c_str(), channel_->fd(), (int)state_);
}

void TcpConnection::send(const std::string &buf)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(buf.c_str(), buf.size());
        }
        else
        {
            loop_->runInLoop(std::bind(
                &TcpConnection::sendInLoop,
                this,
                buf.c_str(),
                buf.size()
			));
        }
    }
}

void TcpConnection::sendInLoop(const void *data, size_t len)
{
    ssize_t nwrote = 0; // 调用write后已发送的数据长度
    size_t remaining = len; // 待发送的数据长度
    bool faultError = false; // 出现错误

    // 之前调用过connection的shutdown
    if (state_ == kDisconnected)
    {
        LOG_ERROR("disconnected, give up writing! \n");
        return;
    }

    // channel_不在发送数据，而且缓冲没有之前的待发送数据
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = ::write(channel_->fd(), data, len);
        if (nwrote >= 0) // 完全发送完
        {
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCallback_) // 注册过回调
            {
                //
                loop_->queueInLoop(
                    std::bind(writeCompleteCallback_, shared_from_this())
				);
            }
        }
        else // nwrote < 0
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK) // ewouldblock是非阻塞下没有数据的正常返回
            {
                LOG_ERROR("TcpConnection::sendInLoop \n");
                if (errno == EPIPE || errno == ECONNRESET) // 接收到对端socket重置
                {
                    faultError = true; // 错误发生
                }
            }
        }
    }

    /**
     * 当前write没有把数据全部发送，需要把剩余数据保存缓冲区，然后给channel注册
     * epollout事件，poller发现tcp的发送缓冲区有空间，会通知相应的sock即channel，执行回调
     * 有write事件就调用TcpConnection::handleWrite，把发送缓冲区的数据全部发送
    */
    if (!faultError && remaining > 0)
    {
        size_t oldLen = outputBuffer_.readableBytes(); // 发送缓冲区之前剩余的待发送数据长度
        if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_) // 超过高水位
        {
            loop_->queueInLoop(
                std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining)
            );
        }
        outputBuffer_.append((char*)data + nwrote, remaining); // 缓冲区已发送一部分，添加data剩余的到write缓冲区
        if (!channel_->isWriting())
        {
            channel_->enableWriting(); // 这里要注册channel感兴趣的写事件，否则poller无法通知channel关于epollout
        }
    }
}

void TcpConnection::shutdown()
{
    if (state_ == kConnected)
    {
        setState(kDisconnecting);
        loop_->runInLoop(
            std::bind(&TcpConnection::shutdownInLoop, this)
        );
    }
}

void TcpConnection::shutdownInLoop()
{
    if (!channel_->isWriting()) // 说明outputBuffer中的数据已经全部发送
    {
        socket_->shutdownWrite();
    }
}

void TcpConnection::connectEstablished()
{
    setState(kConnected);
    // 防止channel正在执行TcpConnection给它注册的回调对象时，TcpConnection异常地没有了
    // 因为TcpConnection直接给到用户，其状态不可控
    channel_->tie(shared_from_this());
    channel_->enableReading(); // 注册epollIn事件

    // 执行新连接建立的回调
    connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed()
{
    if (state_ == kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll();
        connectionCallback_(shared_from_this());
    }
    channel_->remove();
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0)
    {
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if (n == 0)
    {
        handleClose();
    }
    else
    {
        errno = savedErrno;
        LOG_ERROR("Tcp Connection::handleRead");
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    if (channel_->isWriting())
    {
        int savedErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &savedErrno);
        if (n > 0)
        {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0)
            {
                channel_->disableWriting();
                if (writeCompleteCallback_)
                {
                    loop_->queueInLoop(
                        std::bind(writeCompleteCallback_, shared_from_this())
					);
                }
                // 发送完发现state_为kDisconnecting，则发送过程中有个地方数据没有发送完
                // 就调用了shutdown，而且没有真正shutdown
                if (state_ == kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else
        {
            LOG_ERROR("TcpConnection::handleWrite \n");
        }
    }
    else // 不可写
    {
        LOG_ERROR("TcpConnection fd=%d is down, no more writing \n", channel_->fd());
    }
}

// poller =>（通知） channel::closeCallback => handleClose
void TcpConnection::handleClose()
{
    LOG_INFO("TcpConnection::handleClose fd=%d state=%d \n", channel_->fd(), (int)state_);
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr connPtr(shared_from_this());
    connectionCallback_(connPtr); // 关闭连接的回调，通知用户连接关闭
    closeCallback_(connPtr); // TcpServer::removeConnection
}

void TcpConnection::handleError()
{
    int optVal;
    socklen_t optlen = sizeof optVal;
    int err = 0;
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optVal, &optlen) < 0)
    {
        err = errno;
    }
    else
    {
        err = optVal;
    }
    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d \n", name_.c_str(), err);
}
