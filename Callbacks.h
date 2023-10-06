// 回调的类型
#pragma once

#include <memory>
#include <functional>

class Buffer;
class TcpConnection;
class Timestamp;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
// 新用户连接的回调
using ConnectionCallback = std::function<void(const TcpConnectionPtr &)>;
// 关闭连接的回调
using CloseCallback = std::function<void(const TcpConnectionPtr &)>;
// 读写结束的回调
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr &)>;
// 已连接用于的读写事件
using MessageCallback = std::function<void(const TcpConnectionPtr &,
										Buffer *,
										Timestamp)>;

using HighWaterMarkCallback = std::function<void (const TcpConnectionPtr&, size_t)>;
