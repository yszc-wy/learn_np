/*
* Author:           yszc-wy@foxmail.com 
* Encoding:			    utf-8
* Description:      
*/
#pragma once

// C系统库头文件

// C++系统库头文件

// 第三方库头文件
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
// 本项目头文件

namespace yszc
{

namespace net
{

class EventLoop;
class EventLoopThread;
// 本命名空间内部类的前向声明
class EventLoopThreadPool : boost::noncopyable
{
 public:
  EventLoopThreadPool(EventLoop* baseLoop);
  ~EventLoopThreadPool();
  void setThreadNum(int numThreads) { numThreads_=numThreads; }
  void start();
  EventLoop* getNextLoop();
 private:
  EventLoop* baseLoop_;
  bool started_;
  int numThreads_;
  int next_;
  // boost::ptr_vector 独占它所包含的对象，因而容器之外的共享指针不能共享所有权
  boost::ptr_vector<EventLoopThread> threads_;
  std::vector<EventLoop*> loops_;
};


}

}


