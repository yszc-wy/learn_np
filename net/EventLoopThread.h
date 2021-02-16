/*
* Author:           yszc-wy@foxmail.com 
* Encoding:			    utf-8
* Description:      使一个eventloop在线程中运行
*/

#include "base/noncopyable.h"
#include "base/Thread.h"
#include "base/Mutex.h"
#include "base/Condition.h"

namespace yszc
{

namespace net
{
class EventLoop;


class EventLoopThread : noncopyable
{
 public:
  EventLoopThread();
  ~EventLoopThread();
  // 启动其他线程并运行loop,成功后返回Eventloop供当前线程操作
  EventLoop* startLoop();

 private:
  // 在线程中运行eventloop
  void threadFunc();

  EventLoop* loop_;
  bool exiting_;
  Thread thread_;
  MutexLock mutex_;
  Condition cond_;
};
}

}

