// socket封装
#pragma once

#include <arpa/inet.h> // 格式转换函数
#include <netinet/in.h> // sockaddr_in
#include <string>

// 封装socket类型 即ip+port+fd
class InetAddress
{
public:
    explicit InetAddress(uint16_t port = 0, std::string ip = "127.0.0.1");
    explicit InetAddress(const sockaddr_in &addr)
        : addr_(addr)
    {}

    std::string toIp() const; // 读出ip
    std::string toIpPort() const; // 读出socket
    uint16_t toPort() const; 

    // 返回sockaddr
    const sockaddr_in* getSockAddr() const {return &addr_;}
    void setSockAddr(const sockaddr_in &addr) { addr_ = addr; }
private:
    sockaddr_in addr_;
};