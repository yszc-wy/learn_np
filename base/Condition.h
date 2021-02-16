/*
* Author:           yszc-wy@foxmail.com 
* Encoding:			    utf-8
* Description:      条件变量
*/
#pragma once

// C系统库头文件

// C++系统库头文件
#include <pthread.h>
// 第三方库头文件

// 本项目头文件
#include "base/Mutex.h"
#include "noncopyable.h"




// 系统结构体或其他命名空间中类的前向声明(注意要包含相应的命名空间)


namespace yszc
{

class Condition : noncopyable
{
 public:
  explicit Condition(MutexLock& mutex)
    : mutex_(mutex)
  {
    MCHECK(pthread_cond_init(&pcond_, NULL));
  }

  ~Condition()
  {
    MCHECK(pthread_cond_destroy(&pcond_));
  }

  void wait()
  {
    // 当线程阻塞在当前位置时,mutex被释放,当重新进入wait就会重新获取wait
    MutexLock::UnassignGuard ug(mutex_);
    // 调用此函数前mutex必须加锁
    MCHECK(pthread_cond_wait(&pcond_, mutex_.getPthreadMutex()));
  }

  // returns true if time out, false otherwise.
  // pthread_cond_timedwait,等待指定的时间,若不行就返回(false)TIMEOUT
  bool waitForSeconds(double seconds);

  void notify()
  {
    MCHECK(pthread_cond_signal(&pcond_));
  }

  void notifyAll()
  {
    MCHECK(pthread_cond_broadcast(&pcond_));
  }

 private:
  MutexLock& mutex_;
  pthread_cond_t pcond_;
};



} // namespace yszc

