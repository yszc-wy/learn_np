/*
* Author:           yszc-wy@foxmail.com 
* Encoding:			    utf-8
* Description:      
*/
#pragma once

// C系统库头文件
#include <assert.h>
// C++系统库头文件
#include <functional>
// 第三方库头文件

// 本项目头文件
#include "train/Mutex.h"
#include "CountDownLatch.h"

namespace train
{

class Thread
{
  typedef std::function<void()> Threadfunc;
 public:
  Thread(Threadfunc func)
    : pthread_(0),
      counter_(1),
      started_(false),
      joined_(false),
      func_(func)
  {
  }
  ~Thread(){
    pthread_detach(pthread_);
  }
  void join(){
    assert(started_);
    assert(!joined_);
    joined_ = true;
    pthread_join(pthread_,NULL);
  }
  
  class ThreadData{
   public:
    ThreadData(const Threadfunc& func,CountDownLatch* counter)
      :func_(func),counter_(counter)
    {}
    void runInThread(){
      counter_->downCount();
      func_();
    }
    Threadfunc func_;
    CountDownLatch* counter_;
  };
  void start(){
    assert(!started_);
    started_=true;
    ThreadData* data=new ThreadData(func_,&counter_);
    if(pthread_create(&pthread_,NULL,Thread::startThread,data))
    {
      started_=false;
      delete data;
    } else{
      counter_.wait();
    }
  }

  static void* startThread(void *obj){
    ThreadData* data=static_cast<ThreadData*> (obj);
    data->runInThread();
    delete data;
    return NULL;
  }
 private:
  pthread_t pthread_;
  CountDownLatch counter_;
  bool started_;
  bool joined_;
  Threadfunc func_;
};


}

