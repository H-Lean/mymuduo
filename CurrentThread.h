#pragma once

#include <unistd.h>
#include <sys/syscall.h>

namespace CurrentThread
{
    // 全局变量，但每个线程都会有一个自己的拷贝，其修改互不影响
    extern __thread int t_cachedTid;

    void cacheTid(); // 第一次访问后就存到cache

    inline int tid() // 获取tid
    {
        if (__builtin_expect(t_cachedTid == 0, 0)) // 这个语法是语句的优化
        {
            cacheTid(); // t_cachedTid为0表示当前线程的tid还没有获取过
        }
        return t_cachedTid; // 直接返回已访问
    }
}