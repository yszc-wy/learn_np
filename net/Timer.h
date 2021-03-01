/*
* Author:           
* Encoding:			    utf-8
* Description:      负责管理定时器的类型和回调函数,保存要回调的时间,Timer不负责管理timerfd,timerfd是描述符应该由channel管理,功能的实现都在TimerQueue中
*/

#pragma once

#include "base/noncopyable.h"
#include "base/Timestamp.h"
#include "base/Atomic.h"
#include "Callback.h"


namespace yszc
{

namespace net
{

class Timer : noncopyable
{
 public:
  Timer(const TimerCallback& cb, Timestamp when, double interval)
    : callback_(cb),
      // 过期时间
      expiration_(when),
      // 若是定期任务的时间间隔
      interval_(interval),
      repeat_(interval > 0.0),
      // 不仅使用地址作为Timer的唯一标识,还使用全局的序列号来标识防止TimerId串号
      sequence_(s_numCreated_.incrementAndGet())
  { }

  void run() const
  {
    callback_();
  }

  Timestamp expiration() const  { return expiration_; }
  bool repeat() const { return repeat_; }
  int64_t sequence() const { return sequence_; }

  // 若是定期任务就重新计算超时时间
  void restart(Timestamp now);

 private:
  const TimerCallback callback_;
  Timestamp expiration_;
  const double interval_;
  const bool repeat_;
  const int64_t sequence_;

  static AtomicInt64 s_numCreated_;
};

}

}


