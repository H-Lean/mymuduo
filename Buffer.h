// TcpConnection相关，用于未处理完的数据缓存
#pragma once

#include <vector>
#include <string>
#include <algorithm>

/**
 * | prependable bytes | readable bytes | writable bytes |
 * 0       <=   readerIndex  <=    writerIndex   <=    size
 */

// 网络库底层的缓冲器
class Buffer
{
public:
    static const size_t kCheapPrepend = 8; // prependable（前置）的大小
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize)
		, readerIndex_(kCheapPrepend)
		, writerIndex_(kCheapPrepend)
    {}

    // 可读的长度
    size_t readableBytes() const
    {
        return writerIndex_ - readerIndex_;
    }

    // 可写的长度
    size_t writableBytes() const
    {
        return buffer_.size() - writerIndex_;
    }

    size_t prependableBytes() const
    {
        return readerIndex_;
    }

    // 返回buffer中可读数据的首地址
    const char *peek() const
    {
        return begin() + readerIndex_;
    }

    // 读函数
    void retrieve(size_t len)
    {
        if (len < readableBytes())
        {
            readerIndex_ += len;
        }
        else
        {
            retrieveAll();
        }
    }

    // 全部读完
    void retrieveAll()
    {
        readerIndex_ = writerIndex_ = kCheapPrepend; // 全部读完
    }

    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }

    // 以字符串读取
    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len);
        retrieve(len); //
        return result;
    }

    void ensureWriteableBytes(size_t len)
    {
        if (writableBytes() < len) // 没有充足空间就扩容
        {
            makeSpace(len);
        }
    }

    // 把[data, data + len]内存上的数据添加到writable缓冲区上
    void append(const char *data, size_t len)
    {
        ensureWriteableBytes(len);
        std::copy(data, data + len, beginWrite());
        writerIndex_ += len;
    }

    char *beginWrite()
    {
        return begin() + writerIndex_;
    }

    // 从fd上读取数据（读入缓冲）
    ssize_t readFd(int fd, int *saveErrno);
    // 从fd上写数据
    ssize_t writeFd(int fd, int *saveErrno);

private:
    char *begin()
    {
        return &*buffer_.begin(); // 先取迭代器，再取元素，最后首元素地址
    }

    const char *begin() const
    {
        return &*buffer_.begin(); // 先取迭代器，再取元素，最后首元素地址
    }

    // 扩容
    void makeSpace(size_t len)
    {
        if (writableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            buffer_.resize(writerIndex_ + len);
        }
        else // 挪动空间，将readerIndex_前移
        {
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_, begin() + writerIndex_, begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
    }

    std::vector<char> buffer_; // 底层是vector管理，不需要析构
    size_t readerIndex_; // 读完的首址
    size_t writerIndex_; // 写入的尾址
};
