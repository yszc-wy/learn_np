/*
* Author:           
* Encoding:			    utf-8
* Description:      为何要使用TimerId包装Timer的指针? 是为了避免用户的接口头文件过于庞大吗?
*/

#pragma once


#include <stdint.h>

#include "base/copyable.h"

namespace yszc
{

namespace net
{

class Timer;

// 注意是默认可拷贝的
class TimerId : public copyable
{
 public:
  explicit TimerId(Timer* timer = NULL, int64_t seq = 0)
    : timer_(timer),
      sequence_(seq)
  {
  }

  // default copy-ctor, dtor and assignment are okay

  friend class TimerQueue;

  // 注意不允许用户访问value
 private:
  Timer* timer_;
  int64_t sequence_;
};

}


}
