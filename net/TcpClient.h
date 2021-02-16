/*
* Author:           yszc-wy@foxmail.com 
* Encoding:			    utf-8
* Description:      TcpClient,输入指定的连接地址,使用start进行连接,并提供onConnection,onMessage,onWriteComplete回调接口
*/
#pragma once

// C系统库头文件

// C++系统库头文件

// 第三方库头文件
#include "base/noncopyable.h"
#include "base/Mutex.h"
// 本项目头文件
#include "TcpConnection.h"
#include "Callback.h"



// 系统结构体或其他命名空间中类的前向声明(注意要包含相应的命名空间)


namespace yszc
{

namespace net
{

class Connector;
typedef std::shared_ptr<Connector> ConnectorPtr;

class TcpClient: noncopyable
{
 public:
  TcpClient(EventLoop* loop,
            const InetAddress& serverAddr,
            const string& nameArg);
  ~TcpClient();  // force out-line dtor, for std::unique_ptr members.

  void connect();
  void disconnect();
  void stop();

  TcpConnectionPtr connection() const
  {
    MutexLockGuard lock(mutex_);
    return connection_;
  }

  EventLoop* getLoop() const { return loop_; }
  bool retry() const { return retry_; }
  void enableRetry() { retry_ = true; }

  const string& name() const
  { return name_; }

  // before start
  // no thread safe
  void setConnectionCallback(ConnectionCallback cb){
    connectionCallback_=std::move(cb);
  }
  void setMessageCallback(MessageCallback cb){
    messageCallback_=std::move(cb);
  }
  void setWriteCompleteCallback(WriteCompleteCallback cb){
    writeCompleteCallback_=std::move(cb);
  }
 private:
  /// Not thread safe, but in loop
  void newConnection(int sockfd);
  /// Not thread safe, but in loop
  void removeConnection(const TcpConnectionPtr& conn);



  EventLoop* loop_;
  // 使用PTR的原因是tcpclient与connector或connectionptr的生命周期不一定相同,可能会出现tcpclient析构但connector和connectionptr依然存在的情况
  ConnectorPtr connector_;

  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  WriteCompleteCallback writeCompleteCallback_;
  // connector成功调用响应函数时才创建connection
  mutable MutexLock mutex_;
  TcpConnectionPtr connection_ GUARDED_BY(mutex_);
  const string name_;
  bool retry_;
  bool connect_; 
  int nextConnId_;
};

}


}

