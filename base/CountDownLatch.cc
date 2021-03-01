/*
* Encoding:			    utf-8
* Description:      
*/

// 本类头文件
#include "base/CountDownLatch.h"
// C系统库头文件

// C++系统库头文件

// 第三方库头文件

// 本项目头文件

using namespace yszc;

CountDownLatch::CountDownLatch(int count)
  : mutex_(),
    condition_(mutex_),
    count_(count)
{
  
}


void CountDownLatch::wait()
{
  MutexLockGuard lock(mutex_);
  while(count_>0){
    condition_.wait();
  }
}


void CountDownLatch::countDown()
{
  MutexLockGuard lock(mutex_);
  --count_;
  if(count_==0){
    condition_.notifyAll();
  }
}


int CountDownLatch::getCount() const
{
  MutexLockGuard lock(mutex_);
  return count_;
}
