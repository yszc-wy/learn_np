/*
* Encoding:			    utf-8
* Description:      
*/
#pragma once

// C系统库头文件

// C++系统库头文件
#include <deque>
// 第三方库头文件

// 本项目头文件
#include <train/Mutex.h>


namespace train
{

template<typename T>
class BlockQueue
{
 public:
 private: 
  mutable MutexLock mutex_;
  std::deque<T> queue;
};

}

