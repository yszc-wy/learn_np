/*
* Author:           yszc-wy@foxmail.com 
* Encoding:			    utf-8
* Description:      事件循环,事件循环作为一定会被包含的用户接口,其使用的大部分类被前向声明以避免头文件的膨胀,其从属对象使用unique_ptr或scoped_ptr控制生命周期
*/
#pragma once

#include <vector>
#include <functional>

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>


#include "base/CurrentThread.h"
#include "base/Timestamp.h"
#include "base/Mutex.h"

#include "TimerId.h"
#include "Callback.h"

namespace yszc
{

namespace net
{
class Poller;
class Channel;
class TimerQueue;
class EventLoop: boost::noncopyable
{
 public:
  typedef std::function<void()> Functor;

  EventLoop();
  ~EventLoop();
  void loop();
  // 该函数用于保证某些线程非安全的函数必须在当前线程中工作,比如loop函数
  void assertInLoopThread()
  {
    if(!isInLoopThread())
    {
      abortNotInLoopThread();
    }
  }
  // poller最近一次返回的时间
  Timestamp pollReturnTime() const {return pollReturnTime_;}
  // 如果在loop的线程中,会就地运行
  // 若在非loop的线程中,会立即解除poller阻塞并在loop线程中回调cb,有点类似于立即执行的timerqueue
  void runInLoop(const Functor& cb);
  void queueInLoop(const Functor& cb);


  bool isInLoopThread() const {return threadId_==CurrentThread::tid();}
  static EventLoop* getEventLoopOfCurrentThread();
  void quit();

  // 使用wakeupfd唤醒loop线程
  void wakeup();
  void updateChannel(Channel* channel);
  void removeChannel(Channel* channel);
  bool hasChannel(Channel* channel);

  TimerId runAt(const Timestamp& time,const TimerCallback& cb);
  TimerId runAfter(double delay,const TimerCallback& cb);
  TimerId runEvery(double interval,const TimerCallback& cb);

  // 使用TimerId来标识时间任务
  void cancel(TimerId timerId);
  
 private:
  typedef std::vector<Channel*> ChannelList;
  void abortNotInLoopThread();
  // channel回调,用于消除唤醒状态
  void handleRead();
  // 在loop循环末尾调用
  void doPendingFunctors();
  // EventLoop 状态位,提醒类的当前状态,多线程访问
  bool looping_; 
  bool quit_;
  // 一般用于检查当前回调函数是否执行在doPendingFunction函数的内部
  bool callingPendingFunctors_; 

  const pid_t threadId_;
  Timestamp pollReturnTime_;
  // 由于poller头文件与eventloop头文件相互引用包含的问题,必须使用前向声明+指针,由于Eventloop又持有生命周期,所以使用scoped_ptr指针保证声明周期
  boost::scoped_ptr<Poller> poller_;
  // 同样不持有ChannelList的生命周期
  std::unique_ptr<TimerQueue> timerQueue_;
  ChannelList activeChannels_;

  // eventloop的wakeup功能成员
  int wakeupFd_;
  // 仅仅作为wakeupfd与poller的接口,回调函数的注册和执行都是由Eventloop负责
  boost::scoped_ptr<Channel> wakeupChannel_;

  // runInloop功能成员
  MutexLock mutex_;
  std::vector<Functor> pendingFunctors_; // @GuardedBy mutex_
};

} // namespace net

} // namespace yszc

