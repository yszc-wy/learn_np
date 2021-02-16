/*
* Author:           yszc-wy@foxmail.com 
* Encoding:			    utf-8
* Description:      sockaddr_in4/6的包装,属于POD类型,非对象,处理ipv4,ipv6的版本和tostring等问题
*/
#pragma once

// C系统库头文件
#include<netinet/in.h>
// C++系统库头文件
#include <string>
// 第三方库头文件
#include "base/copyable.h"
#include "base/StringPiece.h"
// 本项目头文件




// 其他命名空间类的前向声明


namespace yszc
{

// 本命名空间内部类的前向声明

namespace net
{


namespace sockets
{
const struct sockaddr* sockaddr_cast(const struct sockaddr_in6* addr);
}

class InetAddress : public copyable
{
 public:
  /// Constructs an endpoint with given port number.
  /// Mostly used in TcpServer listening.
  explicit InetAddress(uint16_t port = 0, bool loopbackOnly = false, bool ipv6 = false);

  /// Constructs an endpoint with given ip and port.
  /// @c ip should be "1.2.3.4"
  InetAddress(StringArg ip, uint16_t port, bool ipv6 = false);

  /// Constructs an endpoint with given struct @c sockaddr_in
  /// Mostly used when accepting new connections
  InetAddress(const struct sockaddr_in& addr)
    : addr_(addr)
  { }

  explicit InetAddress(const struct sockaddr_in6& addr)
    : addr6_(addr)
  { }

  // 查看协议版本
  sa_family_t family() const { return addr_.sin_family; }
  std::string toIp() const;
  std::string toIpPort() const;
  uint16_t toPort() const;
  std::string toHostPort() const;
  // std::string toIpPort() const ;

  // default copy/assignment are Okay

  // const struct sockaddr_in& getSockAddrInet() const { return addr_; }

  const struct sockaddr* getSockAddr() const { return sockets::sockaddr_cast(&addr6_); }
  // void setSockAddrInet(const struct sockaddr_in& addr) { addr_ = addr; }
  void setSockAddrInet6(const struct sockaddr_in6& addr6) { addr6_ = addr6; }

  uint32_t ipNetEndian() const;
  uint16_t portNetEndian() const { return addr_.sin_port; } // 用到了ipv4/6结构体中port的偏移量时一样的情况,所以需要事先检查偏移量

  // resolve hostname to IP address, not changing port or sin_family
  // return true on success.
  // thread safe
  static bool resolve(StringArg hostname, InetAddress* result);

  // set IPv6 ScopeID ipv6专用
  void setScopeId(uint32_t scope_id);
 private:
  // union的大小是二者的较大值,也就是sizeof sockaddr_in6
  union
  {
    struct sockaddr_in addr_;
    struct sockaddr_in6 addr6_;
  };
};
}
}

