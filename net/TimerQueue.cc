/*
* Author:           yszc-wy@foxmail.com 
* Encoding:			    utf-8
* Description:      
*/

#include "TimerQueue.h"

#include <sys/timerfd.h>

#include <functional>
#include <utility>
#include <iterator>

#include "base/Logging.h"

#include "EventLoop.h"
#include "Channel.h"
#include "TimerId.h"
#include "Timer.h"

namespace yszc{
namespace detail{
// 处于detail域的都是该unit专有的外部工具函数
int createTimerfd()
{
  int timerfd = ::timerfd_create(CLOCK_MONOTONIC,
                                 TFD_NONBLOCK | TFD_CLOEXEC);
  if (timerfd < 0)
  {
    LOG_SYSFATAL << "Failed in timerfd_create";
  }
  return timerfd;
}

struct timespec howMuchTimeFromNow(Timestamp when)
{
  int64_t microseconds = when.microSecondsSinceEpoch()
                         - Timestamp::now().microSecondsSinceEpoch();
  if (microseconds < 100)
  {
    microseconds = 100;
  }
  struct timespec ts;
  ts.tv_sec = static_cast<time_t>(
      microseconds / Timestamp::kMicroSecondsPerSecond);
  ts.tv_nsec = static_cast<long>(
      (microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
  return ts;
}

void readTimerfd(int timerfd, Timestamp now)
{
  uint64_t howmany;
  ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
  LOG_TRACE << "TimerQueue::handleRead() " << howmany << " at " << now.toString();
  if (n != sizeof howmany)
  {
    LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
  }
}

// 将队列中最早的超时时间用于重置timefd
void resetTimerfd(int timerfd, Timestamp expiration)
{
  // wake up loop by timerfd_settime()
  struct itimerspec newValue;
  struct itimerspec oldValue;
  bzero(&newValue, sizeof newValue);
  bzero(&oldValue, sizeof oldValue);
  newValue.it_value = howMuchTimeFromNow(expiration);
  int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
  if (ret)
  {
    LOG_SYSERR << "timerfd_settime()";
  }
}

}
}

using namespace yszc;
using namespace yszc::net;
using namespace yszc::detail;

TimerQueue::TimerQueue(EventLoop* loop)
  : loop_(loop),
    timerfd_(yszc::detail::createTimerfd()),
    // 注意顺序,否则可能会把未初始化的fd赋予channel
    timerfdChannel_(loop,timerfd_),
    timers_(),
    callingExpiredTimers_(false)
{
  // 设置回调
  timerfdChannel_.setReadCallback(
      std::bind(&TimerQueue::handleRead, this));
  // 绑定到poller
  timerfdChannel_.enableReading();
}


// 析构函数和构造函数在一起,方便释放忘记的资源
TimerQueue::~TimerQueue()
{
  ::close(timerfd_);
  for (TimerList::iterator it = timers_.begin();
      it != timers_.end(); ++it)
  {
    delete it->second;
  }
  // 是否需要删除Poller里的记录? 不需要,因为loop中TimerQueue的生命周期和Poller相同
}


// 负责注册时间事件,会创建timer并加入到时间队列中(并负责管理Timer的生命周期),必须是线程安全的
TimerId TimerQueue::addTimer(const TimerCallback& cb,
                 Timestamp when,
                 double interval)
{
  // 创建timer
  Timer* timer = new Timer(cb, when, interval);
  // TimerPtr timer_ptr(timer);
  loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop,this,timer)); 
  // 这里虽然将原始指针传递给TimerId,但TimerId这个类不为用户提供访问指针的接口,仅作id使用,所以不必担心会让用户访问到空指针
  return TimerId(timer,timer->sequence());
}

void TimerQueue::addTimerInLoop(Timer* timer)
{
  // 都需要判断是否在loop线程上
  loop_->assertInLoopThread();
  // 加入到时间队列中
  bool earliestChanged = insert(timer);

  if (earliestChanged)
  {
    resetTimerfd(timerfd_, timer->expiration());
  }
}


void TimerQueue::cancel(TimerId timerId)
{
  loop_->runInLoop(
      std::bind(&TimerQueue::cancelInLoop, this, timerId));
}


void TimerQueue::cancelInLoop(TimerId timerId)
{
  loop_->assertInLoopThread();
  assert(timers_.size() == activeTimers_.size());
  ActiveTimer timer(timerId.timer_, timerId.sequence_);
  ActiveTimerSet::iterator it = activeTimers_.find(timer);
  // 如果现有的表中存在该timer,就立即删除
  if (it != activeTimers_.end())
  {
    size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
    assert(n == 1); (void)n;
    delete it->first; 
    activeTimers_.erase(it);
  }
  // 有一种可能,就是虽然表中没有timer,但由于现在正在执行timer call,要被取消的timer现在处于expired数组中,而且该timer是repeat的,这会导致该timer会在timer call执行完之后的reset中重新加入timers队列中,所以现有表中不存在,且在执行timer call时,我们要先记住这些timer,然后在reset时检查timer是否已经被取消,若已经取消则在reset中删除
  // 如果正在调用超时的timer call,就是cancelInloop在timer_call内部被调用
  else if (callingExpiredTimers_)
  {
    cancelingTimers_.insert(timer);
  }
  assert(timers_.size() == activeTimers_.size());
}


// 对所有的过期timer执行handle,而不是只执行头结点的handle
void TimerQueue::handleRead()
{
  loop_->assertInLoopThread();
  Timestamp now(Timestamp::now());
  // 消除唤醒状态,否则下一次还会激活,造成eventloop忙等待
  readTimerfd(timerfd_, now);

  std::vector<Entry> expired = getExpired(now);

  callingExpiredTimers_ = true;
  cancelingTimers_.clear();
  // safe to callback outside critical section
  for (std::vector<Entry>::iterator it = expired.begin();
      it != expired.end(); ++it)
  {
    it->second->run();
  }
  callingExpiredTimers_ = false;
  // 对于可重复的timer,需要重置然后再次加入queue
  reset(expired, now);
}


std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)
{
  assert(timers_.size() == activeTimers_.size());
  std::vector<Entry> expired;


  // 由于pair先比较timestamp再比较指针大小,所有我们将指针大小设置为最大,此时lowbound找到的位置应该是timestamp为now区间尾部的下一个位置(要让所有timestamp==now的timer都过期并执行),所以下面的断言才有now<it->first
  Entry sentry = std::make_pair(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
  // Entry sentry =std::make_pair(now, std::make_shared<Timer>(reinterpret_cast<Timer*>(UINTPTR_MAX))) ;
  // 这里unique_ptr不会报错,应该是move
  // Entry sentry =std::make_pair(now, std::unique_ptr<Timer>(reinterpret_cast<Timer*>(UINTPTR_MAX)));

  TimerList::iterator it = timers_.lower_bound(sentry);
  // 由于getexpired在触发时间调用
  assert(it == timers_.end() || now < it->first);
  // FIXME unique_ptr无法被复制,这里应该是所有权转让
  // std::copy(timers_.begin(), it, back_inserter(expired));
  // for(auto it =timers_.begin();it!=it_end;){
    // // 先release unique_ptr,然后插入到expired中,不可行,因为set拒绝非const访问(无法修改key),对于set中的unique_ptr,一旦插入到set就拿不出来了,资源会被完全锁死在set中,一旦set调用erase,就会释放unique_ptr所持有的资源
    // expired.push_back(std::make_pair(it->first,it->second.release()))
    // // 注意这里先获取下一个it,因为set按照迭代器遍历是有序的,且删除操作不会导致set本身失效
    // timers_.erase(it++);
  // }
  // std::move(timers_.begin(),it_end,std::back_inserter(expired));

  std::copy(timers_.begin(), it, back_inserter(expired));
  // 删除处于未决状态的timers
  timers_.erase(timers_.begin(), it);

  for(const Entry& entry: expired)
  {
    ActiveTimer timer(entry.second, entry.second->sequence());
    size_t n = activeTimers_.erase(timer);
    assert(n == 1); (void)n;
  }
  assert(timers_.size() == activeTimers_.size());
  return expired;
}

 
// 需要将repeat的unique_ptr释放,这里expired不可为const
// 遍历过时list,获取存在repeat标记的timer,执行restart,并插入到timer中
void TimerQueue::reset(const std::vector<Entry>& expired, Timestamp now)
{
  Timestamp nextExpire;

  // 同理这里应该也是非const的迭代器
  for (std::vector<Entry>::const_iterator it = expired.begin();
      it != expired.end(); ++it)
  {
    ActiveTimer timer(it->second, it->second->sequence());
    // 在取消集合里面应该找不到该timer
    if (it->second->repeat()
        && cancelingTimers_.find(timer) == cancelingTimers_.end())
    {
      it->second->restart(now);
      insert(it->second);
    }
    else
    {
      // FIXME move to a free list
      delete it->second;
    }
  }

  if (!timers_.empty())
  {
    nextExpire = timers_.begin()->second->expiration();
  }

  // ?
  if (nextExpire.valid())
  {
    resetTimerfd(timerfd_, nextExpire);
  }
}




bool TimerQueue::insert(Timer* timer)
{
  loop_->assertInLoopThread();
  assert(timers_.size() == activeTimers_.size());
  bool earliestChanged = false;
  Timestamp when = timer->expiration();
  TimerList::iterator it = timers_.begin();
  // 如果插入的timer排在第一位,或队列为空,说明该timer应该最早注册到timerfd中
  if (it == timers_.end() || when < it->first)
  {
    earliestChanged = true;
  }
  {
    // insert临时对象是move,不会有copy操作
    std::pair<TimerList::iterator, bool> result =
            timers_.insert(Entry(when,timer));
    // 保证成功插入
    assert(result.second); (void)result;
  }
  {
    std::pair<ActiveTimerSet::iterator, bool> result
      = activeTimers_.insert(ActiveTimer(timer, timer->sequence()));
    assert(result.second); (void)result;
  }
  assert(timers_.size() == activeTimers_.size());
  return earliestChanged;
}



