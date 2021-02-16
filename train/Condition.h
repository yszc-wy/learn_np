/*
* Author:           yszc-wy@foxmail.com 
* Encoding:			    utf-8
* Description:      
*/
#pragma once

// C系统库头文件

// C++系统库头文件

// 第三方库头文件

// 本项目头文件
#include "train/Mutex.h"

namespace train
{
  
class Condition
{ 
 public:
  Condition(MutexLock& mutex)
    : mutex_(mutex)
  {
    pthread_cond_init(&cond_,NULL);
  }

  ~Condition(){
    pthread_cond_destroy(&cond_);
  }

  void wait(){
    pthread_cond_wait(&cond_,mutex_.getLock());
  }

  void notify(){
    pthread_cond_signal(&cond_);
  }

  void notifyAll(){
    pthread_cond_broadcast(&cond_);
  }
     
 private:
  MutexLock& mutex_;
  pthread_cond_t cond_;
};

}

