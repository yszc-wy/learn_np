/*
* Author:           yszc-wy@foxmail.com 
* Encoding:			    utf-8
* Description:      
*/
#pragma once

// C系统库头文件

// C++系统库头文件
#include <vector>
#include <deque>
// 第三方库头文件

// 本项目头文件
#include "base/Thread.h"
#include "base/Mutex.h"
#include "base/Condition.h"
#include "base/Atomic.h"
#include "base/Exception.h"


// 系统结构体或其他命名空间中类的前向声明(注意要包含相应的命名空间)


namespace yszc
{

class ThreadPool
{
 public:
  typedef std::function<void()> Task;
  
  ThreadPool(size_t maxSize)
    : mutex_(),
      notFull_(mutex_),
      notEmpty_(mutex_),
      maxSize_(maxSize),
      running_(false)
  {
  }

  ~ThreadPool(){
    if(running_){
      stop();
    }
  }


  void start(int numThreads){
    assert(threads_.empty());
    running_=true;
    for(int i=0;i<numThreads;++i){
      std::unique_ptr<Thread> threadptr;
      threadptr.reset(new Thread(std::bind(&ThreadPool::runInThread,this)));
      threadptr->start();
      threads_.push_back(std::move(threadptr));
    }
  }

  void stop(){
    {
      MutexLockGuard lock(mutex_);
      running_=false;
      notFull_.notify();
      notEmpty_.notify();
    }
    
    for(auto& thr:threads_){
      thr->join();
    }
  }


  void addTask(Task task){
    if(threads_.empty()){
      task();
    } else{
      MutexLockGuard lock(mutex_);
      while(tasks_.size()>=maxSize_&&running_){
        notFull_.wait();
      }
      if(!running_) return ;
      assert(tasks_.size()<maxSize_);
      tasks_.push_back(task);
      notEmpty_.notify();
    }
  }

  void runInThread(){
    try
    {
      while(true){
        Task task;
        {
          MutexLockGuard lock(mutex_);
          while(tasks_.empty()&&running_){
            notEmpty_.wait();
          }
          // 解除阻塞还有stop结束运行这种情况,需要我们进行特判
          if(!tasks_.empty()){
            task=tasks_.front();
            tasks_.pop_front();
            if( maxSize_>0 ) notFull_.notify();
          }
        }      
        task();
      }
    }
    catch (const Exception& ex)
    {
      fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
      fprintf(stderr, "reason: %s\n", ex.what());
      fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
      abort();
    }
    catch (const std::exception& ex)
    {
      fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
      fprintf(stderr, "reason: %s\n", ex.what());
      abort();
    }
    catch (...)
    {
      fprintf(stderr, "unknown exception caught in ThreadPool %s\n", name_.c_str());
      throw; // rethrow
    }
  }
  

  
 private:
  std::vector<std::unique_ptr<Thread>> threads_;
  MutexLock mutex_;
  Condition notFull_ GUARDED_BY(mutex_);
  Condition notEmpty_ GUARDED_BY(mutex_);
  std::deque<Task> tasks_ GUARDED_BY(mutex_);
  size_t maxSize_;
  std::atomic<bool> running_;
}

}


