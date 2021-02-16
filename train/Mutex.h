/*
* Author:           yszc-wy@foxmail.com 
* Encoding:			    utf-8
* Description:      
*/
#pragma once

// C系统库头文件
#include <pthread.h>
// C++系统库头文件

// 第三方库头文件

// 本项目头文件


namespace train
{

class MutexLock
{
 public:
  MutexLock()
  {
    pthread_mutex_init(&mutex_,NULL);
  }
  ~MutexLock()
  {
    pthread_mutex_destroy(&mutex_);
  }
  void lock(){
    pthread_mutex_lock(&mutex_);
  }
  void unlock(){
    pthread_mutex_unlock(&mutex_);
  }
  pthread_mutex_t* getLock(){
    return &mutex_;
  }
 private:
  pthread_mutex_t mutex_;
};


class MutexLockGuard
{
 public:
  MutexLockGuard(MutexLock& mutex)
    :mutex_(mutex)
  {
    mutex_.lock();
  }
  ~MutexLockGuard(){
    mutex_.unlock();
  }
 private:
  // 只需要持有引用即可
  MutexLock &mutex_;
};


}

