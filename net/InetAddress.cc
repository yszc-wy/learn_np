/*
* Author:           yszc-wy@foxmail.com 
* Encoding:			    utf-8
* Description:      
*/

// 本类头文件
#include "InetAddress.h"
// C系统库头文件
#include <netinet/in.h>
#include <strings.h>  // bzero
#include <assert.h>
// C++系统库头文件
// 第三方库头文件
#include <boost/static_assert.hpp>
// 本项目头文件
#include "SocketsOps.h"
#include "base/Types.h"

using namespace yszc;
using namespace yszc::net;


// INADDR_ANY use (type)value casting.
#pragma GCC diagnostic ignored "-Wold-style-cast"
static const in_addr_t kInaddrAny = INADDR_ANY;
static const in_addr_t kInaddrLoopback = INADDR_LOOPBACK;
#pragma GCC diagnostic error "-Wold-style-cast"

//     /* Structure describing an Internet socket address.  */
//     struct sockaddr_in {
//         sa_family_t    sin_family; /* address family: AF_INET */
//         uint16_t       sin_port;   /* port in network byte order */
//         struct in_addr sin_addr;   /* internet address */
//     };

//     /* Internet address. */
//     typedef uint32_t in_addr_t;
//     struct in_addr {
//         in_addr_t       s_addr;     /* address in network byte order */
//     };

//     struct sockaddr_in6 {
//         sa_family_t     sin6_family;   /* address family: AF_INET6 */
//         uint16_t        sin6_port;     /* port in network byte order */
//         uint32_t        sin6_flowinfo; /* IPv6 flow information */
//         struct in6_addr sin6_addr;     /* IPv6 address */
//         uint32_t        sin6_scope_id; /* IPv6 scope-id */
//     };


// 安全检查
static_assert(sizeof(InetAddress) == sizeof(struct sockaddr_in6),
              "InetAddress is same size as sockaddr_in6");
static_assert(offsetof(sockaddr_in, sin_family) == 0, "sin_family offset 0");
static_assert(offsetof(sockaddr_in6, sin6_family) == 0, "sin6_family offset 0");
static_assert(offsetof(sockaddr_in, sin_port) == 2, "sin_port offset 2");
static_assert(offsetof(sockaddr_in6, sin6_port) == 2, "sin6_port offset 2");



// 可以选择初始化ipv4还是ipv6
InetAddress::InetAddress(uint16_t port, bool loopbackOnly, bool ipv6)
{
  static_assert(offsetof(InetAddress, addr6_) == 0, "addr6_ offset 0");
  static_assert(offsetof(InetAddress, addr_) == 0, "addr_ offset 0");
  if (ipv6)
  {
    memZero(&addr6_, sizeof addr6_);
    addr6_.sin6_family = AF_INET6;
    in6_addr ip = loopbackOnly ? in6addr_loopback : in6addr_any;
    addr6_.sin6_addr = ip;
    addr6_.sin6_port = sockets::hostToNetwork16(port);
  }
  else
  {
    memZero(&addr_, sizeof addr_);
    addr_.sin_family = AF_INET;
    in_addr_t ip = loopbackOnly ? kInaddrLoopback : kInaddrAny;
    addr_.sin_addr.s_addr = sockets::hostToNetwork32(ip);
    addr_.sin_port = sockets::hostToNetwork16(port);
  }
}


InetAddress::InetAddress(StringArg ip, uint16_t port, bool ipv6)
{
  if (ipv6)
  {
    memZero(&addr6_, sizeof addr6_);
    sockets::fromIpPort(ip.c_str(), port, &addr6_);
  }
  else
  {
    memZero(&addr_, sizeof addr_);
    sockets::fromIpPort(ip.c_str(), port, &addr_);
  }
}



string InetAddress::toIpPort() const
{
  char buf[64] = "";
  sockets::toIpPort(buf, sizeof buf, getSockAddr());
  return buf;
}

string InetAddress::toIp() const
{
  char buf[64] = "";
  sockets::toIp(buf, sizeof buf, getSockAddr());
  return buf;
}


uint32_t InetAddress::ipNetEndian() const
{
  assert(family() == AF_INET); // 需要保证是ipv4
  return addr_.sin_addr.s_addr;
}

uint16_t InetAddress::toPort() const
{
  return sockets::networkToHost16(portNetEndian());
}


std::string InetAddress::toHostPort() const
{
  char buf[32];
  sockets::toHostPort(buf, sizeof buf, addr_);
  return buf;
}




