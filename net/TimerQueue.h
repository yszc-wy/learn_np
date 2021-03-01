/*
* Author:           
* Encoding:			    utf-8
* Description:      持有回调接口的channel(注册回调),并负责timer的注册和fd的next时间的更新,管理Eventloop的所有时间回调事件
*/
#pragma once

#include <memory>
#include <set>
#include <vector>

#include <boost/noncopyable.hpp>

#include "base/Timestamp.h"
#include "Channel.h"
#include "TimerId.h"
#include "Callback.h"
#include "Channel.h"
namespace yszc
{

namespace net
{

class EventLoop;
class TimerQueue: boost::noncopyable
{
 public:
  TimerQueue(EventLoop* loop);
  ~TimerQueue();

  // 负责注册时间事件,会创建timer并加入到时间队列中(并负责管理Timer的生命周期),必须是线程安全的
  // 还能让用户获取生成的timer接口timerid
  // resource
  TimerId addTimer(const TimerCallback& cb,
                   Timestamp when,
                   double interval);

  void cancel(TimerId timerId);
 private:  

  // 使用set的原因:默认按key排序,取出的第一个数就是timerfd的下一个值,为了防重复使用pair存储
  // 使用share_ptr来管理addTimer创建的Timer对象在合适的地方析构
  // typedef std::shared_ptr<Timer> TimerPtr;
  typedef std::pair<Timestamp, Timer*> Entry;
  typedef std::set<Entry> TimerList;
  typedef std::pair<Timer*, int64_t> ActiveTimer;
  typedef std::set<ActiveTimer> ActiveTimerSet;

  void addTimerInLoop(Timer* timer);
  void cancelInLoop(TimerId timerId);
  // 回调超时的timer的函数
  void handleRead();
  // timerfd触发后,从timerqueue中获取过期的timer
  std::vector<Entry> getExpired(Timestamp now);
  // 对于超时timer中的可以repeat的部分,重新调整后再加入到timequeue中
  void reset(const std::vector<Entry>& expired, Timestamp now);

  // 使用const TimerPtr来避免传参时不必要的拷贝,这个const是针对share指针本身的,不是指针指向的数据
  bool insert(Timer* timer);

  EventLoop* loop_;
  // resource
  const int timerfd_; // 只需要一个timerfd即可
  Channel timerfdChannel_;
  // Timer list sorted by expiration
  TimerList timers_;

  // for cancel()
  bool callingExpiredTimers_; /* atomic */
  // activeTimers_与TimerList同步,方便根据timer指针和seq来快速确定指定的timer是否存在
  // 如果在activeTimers_上找到了相应的timer,说明该timer依然存在timerid不是悬空指针
  ActiveTimerSet activeTimers_;
  ActiveTimerSet cancelingTimers_;
};

}

}


