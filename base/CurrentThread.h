/*
* Encoding:			    utf-8
* Description:      可以获取每个线程独有的一些全局变量,如线程id,线程名如线程id,线程名之类的
*/
#pragma once

// C系统库头文件

// C++系统库头文件

// 第三方库头文件

// 本项目头文件
#include "base/Types.h"



// 系统结构体或其他命名空间中类的前向声明(注意要包含相应的命名空间)


namespace yszc
{
namespace CurrentThread
{
  // CurrentThread.cc的内容在这里做外部声明
  extern __thread int t_cachedTid;
  extern __thread char t_tidString[32];
  extern __thread int t_tidStringLength;
  extern __thread const char* t_threadName;
  void cacheTid();

  // 获取线程id,如果是第一次调用,会进行系统调用,后续调用会直接从t_cachedTid中取,降低消耗
  inline int tid()
  {
    // __builtin_expect允许程序员将最有可能执行的分支告诉编译器
    if (__builtin_expect(t_cachedTid == 0, 0))
    {
      cacheTid();
    }
    return t_cachedTid;
  }

  inline const char* tidString() // for logging
  {
    return t_tidString;
  }

  inline int tidStringLength() // for logging
  {
    return t_tidStringLength;
  }

  inline const char* name()
  {
    return t_threadName;
  }

  bool isMainThread();

  void sleepUsec(int64_t usec);  // for testing

  string stackTrace(bool demangle);

}
}

