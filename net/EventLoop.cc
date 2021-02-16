#include "EventLoop.h"
#include <poll.h>
#include <sys/eventfd.h>
#include <sys/signal.h>

#include "base/Logging.h"
#include "net/Poller.h"
#include "net/Channel.h"
#include "net/TimerQueue.h"

using namespace yszc;
using namespace yszc::net;

// 每个线程内都有一个这样的实例
__thread EventLoop* t_loopInThisThread=0;
const int kPollTimeMs = 10000;

static int createEventfd()
{
  int evtfd=::eventfd(0,EFD_NONBLOCK|EFD_CLOEXEC);
  if(evtfd<0)
  {
    LOG_SYSERR<<"Failed in eventfd";
    abort();
  }    
  return evtfd;
}


class IgnoreSigPipe
{
 public:
  // 忽略SIGPIPE信号
  IgnoreSigPipe()
  {
    ::signal(SIGPIPE, SIG_IGN);
  }
};

IgnoreSigPipe initObj;
EventLoop::EventLoop()
  : looping_(false),
    quit_(false),
    callingPendingFunctors_(false),
    threadId_(CurrentThread::tid()),
    poller_(Poller::newDefaultPoller(this)),
    timerQueue_(new TimerQueue(this)),
    wakeupFd_(createEventfd()),
    wakeupChannel_(new Channel(this,wakeupFd_))
    
{
  LOG_TRACE<<"EventLoop created "<<this<<" in thread "<<threadId_;
  // 若该线程已经有EventLoop对象存在,报错
  if(t_loopInThisThread)
  {
    LOG_FATAL<<"Another EventLoop "<<t_loopInThisThread
      <<" exists in this thread "<<threadId_;
  }
  else
  {
    t_loopInThisThread=this;
  }
  // 向poller中注册wakeupfd
  wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead,this));
  wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
  assert(!looping_);
  LOG_DEBUG << "EventLoop " << this << " of thread " << threadId_
            << " destructs in thread " << CurrentThread::tid();
  wakeupChannel_->disableAll();
  wakeupChannel_->remove();
  ::close(wakeupFd_);
  // 恢复全局线程变量
  t_loopInThisThread=NULL;
}

EventLoop* EventLoop::getEventLoopOfCurrentThread()
{
  return t_loopInThisThread;
}


void EventLoop::quit()
{
  quit_ = true;
  // 立即唤醒指定的线程,防止loop阻塞在poller中导致无法即时退出(可见手动wakeup功能对Eventloop来说是不可缺少的)
  if (!isInLoopThread())
  {
    wakeup();
  }
}

void EventLoop::loop()
{
  // 2个断言保证安全的在本线程调用
  assert(!looping_);
  assertInLoopThread();
  looping_=true;
  // 控制loop的开关
  quit_=false;

  while(!quit_)
  {
    activeChannels_.clear();
    pollReturnTime_=poller_->poll(kPollTimeMs,&activeChannels_);
    for(ChannelList::iterator it=activeChannels_.begin();
        it!=activeChannels_.end();++it)
    {
      (*it)->handleEvent(pollReturnTime_);
    }

    // 在wakeup之后,loop在poller上的阻塞会解除,不循环执行该函数的原因是若pendingfunctor不断调用queueInLoop,会导致doPending死循环,还是要等到下一次调用较好
    doPendingFunctors();
  }


  LOG_TRACE<<"EventLoop "<<this<<" stop looping";
  looping_=false;
}


bool EventLoop::hasChannel(Channel* channel)
{
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  return poller_->hasChannel(channel);
}

void EventLoop::abortNotInLoopThread()
{
  LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
            << " was created in threadId_ = " << threadId_
            << ", current thread id = " <<  CurrentThread::tid();
}


// EventLoop不关心poller是如何管理poll的,只向外提供一个poller的接口
void EventLoop::updateChannel(Channel* channel)
{
  // 保证EventLoop拥有该channel
  assert(channel->ownerLoop()==this);
  // 保证该函数是在创建线程中调用
  assertInLoopThread();
  poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel)
{
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  poller_->removeChannel(channel);
}

TimerId EventLoop::runAt(const Timestamp& time,const TimerCallback& cb)
{
  return timerQueue_->addTimer(cb,time,0.0);
}
TimerId EventLoop::runAfter(double delay,const TimerCallback& cb)
{
  Timestamp time(addTime(Timestamp::now(),delay));
  // 复用
  return runAt(time,cb);
}
TimerId EventLoop::runEvery(double interval,const TimerCallback& cb)
{
  Timestamp time(addTime(Timestamp::now(),interval));
  return timerQueue_->addTimer(cb,time,interval);
}

void EventLoop::cancel(TimerId timerId)
{
  return timerQueue_->cancel(timerId);
}

// 不能在该函数中执行dopendfunction,dopendfunction中可能执行类似delete channel的操作,这会导致Channel在执行handleevent时析构
void EventLoop::handleRead()
{
  uint64_t one = 1;
  ssize_t n = ::read(wakeupFd_, &one, sizeof one);
  if (n != sizeof one)
  {
    LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
  }
}


void EventLoop::wakeup()
{
  uint64_t one = 1;
  ssize_t n = ::write(wakeupFd_, &one, sizeof one);
  if (n != sizeof one)
  {
    LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
  }
}


void EventLoop::doPendingFunctors()
{
  std::vector<Functor> functors;
  callingPendingFunctors_ = true;

  // 使用swap快速清空共享数组,减少临界区大小
  {
    MutexLockGuard lock(mutex_);
    functors.swap(pendingFunctors_);
  }

  // 注意在执行functors时也可能调用Eventloop中的函数,要注意检查这种情况是否合法
  for (size_t i = 0; i < functors.size(); ++i)
  {
    functors[i]();
  }
  callingPendingFunctors_ = false;
}

void EventLoop::runInLoop(const Functor& cb)
{
  if (isInLoopThread())
  {
    cb();
  }
  else
  {
    queueInLoop(cb);
  }
}

void EventLoop::queueInLoop(const Functor& cb)
{
  {
    MutexLockGuard lock(mutex_);
    pendingFunctors_.push_back(cb);
  }

  // callingPendingFunctors_==true,意味着cb中存在queueInLoop的调用
  // FIXME 为何要有这个判断,queueInLoop函数若在本线程的普通回调函数中调用,说明loop函数已经执行到activeChannel的handleEvent循环上,下一步就会执行doPendingFunctors();,因此不必weakup
  // 极端情况:如果在外部线程中调用该eventloop的queueInLoop注册了一个cb,该cb又包含了一个queueInloop,则导致第二个queueInloop是在loop的doPendingFunctors中执行的,这时pendingFunctors中存在一个cb需要写入eventfd以提醒poller不要在下一次loop时阻塞
  // 总结:回调的回调可能产生一些问题,要注意检查
  if (!isInLoopThread() || callingPendingFunctors_)
  {
    wakeup();
  }
}
