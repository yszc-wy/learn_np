/*
* Author:           yszc-wy@foxmail.com 
* Encoding:			    utf-8
* Description:      基于condition实现的计数器
*/
#pragma once

// C系统库头文件

// C++系统库头文件

// 第三方库头文件

// 本项目头文件
#include "base/Condition.h"



// 系统结构体或其他命名空间中类的前向声明(注意要包含相应的命名空间)


namespace yszc
{

class CountDownLatch : noncopyable
{
 public:

  explicit CountDownLatch(int count);

  void wait();

  void countDown();

  int getCount() const;

 private:
  // 顺序很重要
  mutable MutexLock mutex_;
  Condition condition_ GUARDED_BY(mutex_);
  int count_ GUARDED_BY(mutex_);
};


}

