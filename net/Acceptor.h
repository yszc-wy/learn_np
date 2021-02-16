/*
* Author:           yszc-wy@foxmail.com 
* Encoding:			    utf-8
* Description:      Accept在poll中注册listenfd,向其channel的handle_read中注册函数,该函数使用系统accept获取新连接的fd,之后如果操作这个fd目前不知如何处理,就可以在此使用一个可修改的回调函数,这样既有了接口,也不必在未知下一步操作的情况下强行写代码,还降低了耦合,毕竟accept只负责获取新连接的fd
*/
#include <functional>

#include <boost/noncopyable.hpp>

#include "Socket.h"
#include "Channel.h"
namespace yszc
{
class EventLoop;

namespace net
{
class InetAddress;
class Acceptor : boost::noncopyable
{
 public:
  typedef std::function<void (int sockfd,
                                const InetAddress&)> NewConnectionCallback;

  Acceptor(EventLoop* loop, const InetAddress& listenAddr,bool reusePort);
  ~Acceptor();

  void setNewConnectionCallback(const NewConnectionCallback& cb)
  { newConnectionCallback_ = cb; }

  bool listenning() const { return listenning_; }
  void listen();

 private:
  void handleRead();

  EventLoop* loop_;
  Socket acceptSocket_;
  Channel acceptChannel_;
  // 对于accept返回的tcpconnect的sockfd和地址的回调处理
  NewConnectionCallback newConnectionCallback_;
  bool listenning_;
  int idleFd_;
};
}

}

