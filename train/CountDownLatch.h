/*
* Encoding:			    utf-8
* Description:      
*/
#pragma once

// C系统库头文件

// C++系统库头文件

// 第三方库头文件

// 本项目头文件
#include "train/Mutex.h"
#include "train/Condition.h"


namespace train
{

class CountDownLatch
{
 public:
  CountDownLatch(int count)
    : mutex_(),
      cond_(mutex_),
      count_(count)
  {}
  void wait(){
    MutexLockGuard lock(mutex_);
    while(count_>0){
      cond_.wait();
    }
  }
  void downCount(){
    MutexLockGuard lock(mutex_);
    --count_;
    if(count_==0){
      cond_.notifyAll();
    }
  }
 private:
  mutable MutexLock mutex_;
  Condition cond_;
  int count_;
};


}


