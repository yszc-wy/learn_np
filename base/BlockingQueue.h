/*
* Author:           yszc-wy@foxmail.com 
* Encoding:			    utf-8
* Description:      阻塞队列,使用mutex和cond实现
*/
#pragma once

// C系统库头文件

// C++系统库头文件
#include <deque>
// 第三方库头文件

// 本项目头文件
#include "base/Mutex.h"
#include "base/noncopyable.h"
#include "base/Condition.h"


// 系统结构体或其他命名空间中类的前向声明(注意要包含相应的命名空间)


namespace yszc
{

template <typename T>
class BlockingQueue : public noncopyable
{
 public:
  BlockingQueue()
    : mutex_(),
      condition_(mutex_)
  {}
  void put(const T& x)
  {
    MutexLockGuard lock(mutex_);
    queue_.push_back(x);
    condition_.notify();
  }
  void put(T&& x)
  {
    MutexLockGuard lock(mutex_);
    queue_.push_back(std::move(x));
    condition_.notify();
  }
  T take()
  {
    MutexLockGuard lock(mutex_);
    while(queue_.empty()){
      condition_.wait();
    }
    assert(!queue_.empty());
    // 减少拷贝次数
    T front(std::move(queue_.front()));
    queue_.pop_front();
    return front;
  }
  size_t size() const
  {
    MutexLockGuard lock(mutex_);
    return queue_.size();
  }
 private:
  MutexLock mutex_;
  Condition condition_ GUARDED_BY(mutex_);
  std::deque<T>     queue_ GUARDED_BY(mutex_);
};

}

