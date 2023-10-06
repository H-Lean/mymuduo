// 时间戳
#pragma once

#include <iostream>
#include <string>

// 时间类
class Timestamp
{
public:
    Timestamp(); // 默认
    explicit Timestamp(int64_t microSecondsSinceEpoch); // 带参勾走，显式构造，防止其他行为
    static Timestamp now(); // 当前时间
    std::string toString() const; // 转化时间格式
private:
    int64_t microSecondsSinceEpoch_; // 毫秒
};