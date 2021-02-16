/*
* Author:           yszc-wy@foxmail.com 
* Encoding:			    utf-8
* Description:      
*/
#pragma once

// C系统库头文件

// C++系统库头文件

// 第三方库头文件
#include <boost/circular_buffer.hpp>
// 本项目头文件
#include "base/noncopyable.h"
#include "base/Mutex.h"
#include "base/Condition.h"



// 系统结构体或其他命名空间中类的前向声明(注意要包含相应的命名空间)

namespace yszc
{

template <typename T>
class BoundedBlockingQueue : public noncopyable
{
 public:
  BoundedBlockingQueue(int max_size)
    : mutex_(),
      notEmpty_(mutex_),
      notFull_(mutex_),
      queue_(max_size)      
  {}
  void put(const T& x)
  {
    MutexLockGuard lock(mutex_);
    while(queue_.full()){
      notFull_.wait();
    }
    assert(!queue_.full());
    queue_.push_back(x);
    notEmpty_.notify();
  }
  void put(T&& x)
  {
    MutexLockGuard lock(mutex_);
    while(queue_.full()){
      notFull_.wait();
    }
    assert(!queue_.full());
    queue_.push_back(std::move(x));
    notEmpty_.notify();
  }

  T take()
  {
    MutexLockGuard lock(mutex_);
    while(queue_.empty()){
      notEmpty_.wait();
    }
    assert(!queue_.empty());
    T front(std::move(queue_.front()));
    queue_.pop_front();
    notFull_.notify();
    return front;
  }

  bool empty() const
  {
    MutexLockGuard lock(mutex_);
    return queue_.empty();
  }
  bool full() const
  {
    MutexLockGuard lock(mutex_);
    return queue_.full();
  }
  size_t size() const
  {
    MutexLockGuard lock(mutex_);
    return queue_.size();
  }

  size_t capacity() const
  {
    MutexLockGuard lock(mutex_);
    return queue_.capacity();
  }
 private:
  MutexLock mutex_;
  Condition notEmpty_ GUARDED_BY(mutex_);
  Condition notFull_ GUARDED_BY(mutex_);
  boost::circular_buffer<T> queue_ GUARDED_BY(mutex_);
};

}

