/*
* Author:           
* Encoding:			    utf-8
* Description:      负责生成connectfd,并在Eventloop中注册connect事件,当connect成功建立时会调用handlewrite,处理与连接相关的错误,提供了连接成功时的事件注册
* 连接的流程并不是直接就注册connect事件,而是在loop调用connect函数,使用系统connect获取ret,根据不同的返回结果决定是否进行事件注册、重试、
*/
#pragma once

// C系统库头文件

// C++系统库头文件
#include <functional>
#include <memory>
// 第三方库头文件
// 本项目头文件
#include "base/noncopyable.h"
#include "InetAddress.h"
#include "TimerId.h"



// 系统结构体或其他命名空间中类的前向声明(注意要包含相应的命名空间)


namespace yszc
{

namespace net
{
// 本命名空间内部类的前向声明
class EventLoop;
class Channel;

class Connector : noncopyable
{
 public:
  typedef std::function<void (int sockfd)> NewConnectionCallback;
  Connector(EventLoop* loop, const InetAddress& serverAddr);
  ~Connector();

  void setNewConnectionCallback(const NewConnectionCallback& cb)
  { newConnectionCallback_ = cb; }

  void start();  // can be called in any thread
  void restart();  // must called in loop thread
  void stop();  // can be called in any thread

  const InetAddress& serverAddress() const { return serverAddr_; }
 private:
  enum States { kDisconnected, kConnecting, kConnected };
  static const int kMaxRetryDelayMs = 30*1000;
  static const int kInitRetryDelayMs = 500;

  void setState(States s) { state_ = s; }
  void startInLoop();
  void connect();
  void connecting(int sockfd);
  void handleWrite();
  void handleError();
  void retry(int sockfd);
  int removeAndResetChannel();
  void resetChannel();

  EventLoop* loop_;
  InetAddress serverAddr_;
  bool connect_; // atomic
  States state_;  // FIXME: use atomic variable
  std::unique_ptr<Channel> channel_;
  NewConnectionCallback newConnectionCallback_;
  int retryDelayMs_;
  // 用于注册重复连接回调
  TimerId timerId_;

};

typedef std::shared_ptr<Connector> ConnectorPtr;
}


}

