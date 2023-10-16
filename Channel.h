/**
 * Channel 即 通道，封装了sockfd和其感兴趣的event，如EPOLLIN、EPOLLOUT事件，还绑定了poller返回的具体事件
 * EventLoop包含了一个poller和多个channel，相当于多路事件分发器
 * 发生的事件由poller向channel通知，然后注册
*/
#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <functional>
#include <memory> // weak_ptr

class EventLoop; // 先不要include，等到实际使用的时候，OOP思想

class Channel : noncopyable
{
public:
    using EventCallback = std::function<void()>; // C++11的using语法代替typedef
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop *loop, int fd); // 指针则不需要EventLoop具体，都是4个字节
    ~Channel();

    // fd得到poller通知后，处理事件。
    void handleEvent(Timestamp receiveTime); // 需要include Timestamp，因为不是指针

    // 回调函数对象
    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); } // move移动语义，把左值转化为右值引用
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

    // 防止当channel被手动remove后，channel还在执行回调操作
    void tie(const std::shared_ptr<void>&);

    // 返回对象
    int fd() const { return fd_; }
    int events() const { return events_; }
    int index() { return index_; }

    // poller上注册可读事件
    void enableReading() { events_ |= kReadEvent; update(); }

    // 从poller中移除channel的感兴趣事件
    void disableReading() { events_ &= ~kReadEvent; update(); }
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }

    // 返回fd当前的事件状态
    bool isNoneEvent() const { return events_ == kNoneEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const { return events_ & kReadEvent;  }

    // 成员赋值
    void set_revents(int revt) { revents_ = revt; } // poller监听revents，所以channel要提供接口来设置
    void set_index(int idx) { index_ = idx; }

    // one loop per thread
    EventLoop* ownerLoop() { return loop_; }
    // 在channel所属的EventLoop中，删除当前的channel
    void remove();

private:
    // 更新感兴趣的事件，调用loop_的updateChannel()->调用poller的update()
    void update();

    // handleEvent()调用
    void handleEventWithGuard(Timestamp receiveTime);

    // 添加到poller的状态
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_; // 事件循环
    const int fd_; // poller监听对象
    int events_; // 注册fd感兴趣的对象
    int revents_; // poller返回的具体发生的事件
    int index_; // poller使用

    std::weak_ptr<void> tie_; // 弱智能指针
    bool tied_; // 是否绑定

    // 由于channel通道可以获知fd发生的具体事件revents，所以它负责执行具体事件的回调操作
    ReadEventCallback readCallback_; // read事件需要时间戳
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};
