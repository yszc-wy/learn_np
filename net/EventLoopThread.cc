#include "EventLoopThread.h"

#include "EventLoop.h"

using namespace yszc;
using namespace yszc::net;


EventLoopThread::EventLoopThread()
  :loop_(NULL),
   exiting_(false),
   thread_(std::bind(&EventLoopThread::threadFunc,this)),
   mutex_(),
   cond_(mutex_)
{
}

EventLoopThread::~EventLoopThread()
{
  exiting_ = true;
  loop_->quit();
  thread_.join();
}


EventLoop* EventLoopThread::startLoop()
{
  // 一定要确保只start一次
  assert(!thread_.started());
  thread_.start();

  {
    MutexLockGuard lock(mutex_);
    while (loop_ == NULL)
    {
      cond_.wait();
    }
  }

  return loop_;
}


void EventLoopThread::threadFunc()
{
  // 在线程中创建才能在线程中运行,eventloop的规则
  EventLoop loop;

  {
    MutexLockGuard lock(mutex_);
    loop_ = &loop;
    cond_.notify();
  }

  loop.loop();
  //assert(exiting_);
}
