/*
* Author:           
* Encoding:			    utf-8
* Description:      包装sockfd相关的系统函数的接口,以及对sockfd的配置,
* 使用RAII对sockfd进行管理,与sockfd相关的读写操作全部定义在该类中,
* 且禁止拷贝(对象语义),这样可以防止串话,Socket对象存在时,新建的Socket一定
* 不会和其有一样的fd.
*/
#pragma once

// C系统库头文件

// C++系统库头文件

// 第三方库头文件
#include <boost/noncopyable.hpp>

// 本项目头文件



// 系统结构体或其他命名空间中类的前向声明(注意要包含相应的命名空间)

namespace yszc
{

// 本命名空间内部类的前向声明
namespace net
{
class InetAddress;

/*
* Socket使用RAII模式管理fd资源,noncopyable
* 包装sockfd,仅提供配置和系统函数的接口
* sockfd应该被设置为非阻塞
*/
class Socket: boost::noncopyable
{
 public:
  explicit Socket(int sockfd)
    :sockfd_(sockfd)
  {}

  ~Socket();

  int fd() const { return sockfd_; }

  /// abort if address in use
  void bindAddress(const InetAddress& localaddr);

  /// abort if address in use
  void listen();

  // 注意该accept为非阻塞调用,一旦accept成功,就赋予peeraddr,若失败返回-1
  int accept(InetAddress* peeraddr);

  // 一般来说，一个端口释放后会等待两分钟(TCP timewait)之后才能再被使用，SO_REUSEADDR是让端口释放后立即就可以被再次使用
  void setReuseAddr(bool on);
  void setReusePort(bool on);
  void shutdownWrite();

  ///
  /// Enable/disable TCP_NODELAY (disable/enable Nagle's algorithm).
  ///
  void setTcpNoDelay(bool on);
 private:
  const int sockfd_;
};
}


}


