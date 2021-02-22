/*
* Author:           
* Encoding:			    utf-8
* Description:      
*/
#pragma once

#include "net/TcpServer.h"
#include <unordered_set>
#include <boost/circular_buffer.hpp>
#include "net/TimeWheel.h"
#include "base/Mutex.h"



namespace yszc
{

namespace net
{

class HttpRequest;
class HttpResponse;

class HttpServer : noncopyable
{
 public:
  typedef std::function<void (const HttpRequest&,
                              HttpResponse*)> HttpCallback;


  HttpServer(EventLoop* loop,
             const InetAddress& listenAddr,
             const string& name,
             TcpServer::Option option = TcpServer::kNoReusePort);

  EventLoop* getLoop() const { return server_.getLoop(); }

  /// Not thread safe, callback be registered before calling start().
  void setHttpCallback(const HttpCallback& cb)
  {
    httpCallback_ = cb;
  }

  void setThreadNum(int numThreads)
  {
    server_.setThreadNum(numThreads);
  }

  void start();

 private:
  void onConnection(const TcpConnectionPtr& conn);
  void onMessage(const TcpConnectionPtr& conn,
                 Buffer* buf,
                 Timestamp receiveTime);
  void onRequest(const TcpConnectionPtr&, const HttpRequest&);


  void onTimer()
  {
    MutexLockGuard lock(mutex_);
    list_.push_back(TimeWheel::Bucket());
  }

  TcpServer server_;
  HttpCallback httpCallback_;

  
  mutable MutexLock mutex_;
  TimeWheel::BucketList list_ GUARDED_BY(mutex_); 
};


}


}

