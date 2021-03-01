/*
* Author:           
* Encoding:			    utf-8
* Description:      统一的线程创建接口,方便作线程相关的记录工作并对程序创建的线程作统一管理,掌握线程相关的性能指标
* 一个服务器程序的线程数量与当前负载无关,与计算机的cpu数量有关
* 在程序的初始化阶段就创建好所有的线程,在程序运行期间不会额外的创建和销毁线程,这样也可以避免thread对象相关的生命周期管理问题
* Thread的start会创建ThreadData对象,该持有Thread的tid,latch_对象的指针,因为tid只有在线程执行后才可以得到,因此我们向pthread_create中
* 注册start_thread函数,在该函数中调用Thread的runInThread函数来对线程内全局变量和Thread对象的成员进行一些初始化操作,然后运行fun
*/
#pragma once

// C系统库头文件
#include <pthread.h>
// C++系统库头文件
#include <functional>
#include <memory>
// 第三方库头文件

// 本项目头文件
#include "base/Atomic.h"
#include "base/CountDownLatch.h"
#include "base/Types.h"




// 系统结构体或其他命名空间中类的前向声明(注意要包含相应的命名空间)


namespace yszc
{

class Thread : public noncopyable
{
 public:
  typedef std::function<void()> ThreadFunc;

  explicit Thread(ThreadFunc,const string& name=string());
  // FIXME: make it movable in C++11
  ~Thread();

  void start();
  int join(); // return pthread_join()

  bool started() const { return started_; }
  // pthread_t pthreadId() const { return pthreadId_; }
  pid_t tid() const { return tid_; }
  const string& name() const { return name_; }
  static int numCreated() { return numCreated_.get(); }
 private:
  void setDefaultName();

  // 全局变量中也有保存和下面类似的状态
  // 线程状态
  bool       started_;
  bool       joined_;
  // 线程id,其中tid需要创建后从线程中获取,需要进行同步操作
  pthread_t pthreadId_;
  pid_t tid_;
  // 线程函数
  ThreadFunc func_;
  // 线程名
  string name_;
  
  CountDownLatch latch_;
  
  static AtomicInt32 numCreated_;
};

}

