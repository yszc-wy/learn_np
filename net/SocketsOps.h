/*
* Encoding:			    utf-8
* Description:      该类负责包装底层socket相关函数,处理相关函数的错误情况,并设置一些常用sockfd配置相关的工具函数
*/
#pragma once

// C系统库头文件
#include <arpa/inet.h>
#include <endian.h>
// C++系统库头文件

// 第三方库头文件

// 本项目头文件




// 其他命名空间类的前向声明


namespace yszc
{

namespace net
{

namespace sockets
{

inline uint64_t hostToNetwork64(uint64_t host64)
{
  return htobe64(host64);
}

inline uint32_t hostToNetwork32(uint32_t host32)
{
  return htonl(host32);
}

inline uint16_t hostToNetwork16(uint16_t host16)
{
  return htons(host16);
}

inline uint64_t networkToHost64(uint64_t net64)
{
  return be64toh(net64);
}

inline uint32_t networkToHost32(uint32_t net32)
{
  return ntohl(net32);
}

inline uint16_t networkToHost16(uint16_t net16)
{
  return ntohs(net16);
}

// 创建nonblock sockfd,出错则关闭
int createNonblockingOrDie();

// 包装系统函数,进行错误处理
int  connect(int sockfd, const struct sockaddr* addr);
void bindOrDie(int sockfd, const struct sockaddr* addr);
void listenOrDie(int sockfd);
int  accept(int sockfd, struct sockaddr_in6* addr);
ssize_t read(int sockfd, void *buf, size_t count);
ssize_t readv(int sockfd, const struct iovec *iov, int iovcnt);
ssize_t write(int sockfd, const void *buf, size_t count);
void close(int sockfd);
void shutdownWrite(int sockfd);

// 地址格式转换
// sockaddr_in/sockaddr_in6 -> sockaddr
const struct sockaddr* sockaddr_cast(const struct sockaddr_in* addr);
const struct sockaddr* sockaddr_cast(const struct sockaddr_in6* addr);
struct sockaddr* sockaddr_cast(struct sockaddr_in6* addr);
// sockaddr->sockaddr_in/in6
const struct sockaddr_in* sockaddr_in_cast(const struct sockaddr* addr);
const struct sockaddr_in6* sockaddr_in6_cast(const struct sockaddr* addr);

// 从二进制地址到字符串地址
void toHostPort(char* buf, size_t size,
                const struct sockaddr_in& addr);
// 字符串地址到二进制地址
void fromHostPort(const char* ip, uint16_t port,
                  struct sockaddr_in* addr);

void toIpPort(char* buf, size_t size,
              const struct sockaddr* addr);
void toIp(char* buf, size_t size,
          const struct sockaddr* addr);

void fromIpPort(const char* ip, uint16_t port,
                struct sockaddr_in* addr);
void fromIpPort(const char* ip, uint16_t port,
                struct sockaddr_in6* addr);


// struct sockaddr_in getLocalAddr(int sockfd);
// struct sockaddr_in getPeerAddr(int sockfd);

struct sockaddr_in6 getLocalAddr(int sockfd);
struct sockaddr_in6 getPeerAddr(int sockfd);


int getSocketError(int sockfd);
bool isSelfConnect(int sockfd);
}
}



}

