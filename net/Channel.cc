#include "Channel.h"

#include <sys/epoll.h>
#include <poll.h> 
#include <sstream>

#include "base/Logging.h"
#include "EventLoop.h"

using namespace yszc;
using namespace yszc::net;

const int Channel::kNoneEvent=0;
const int Channel::kReadEvent=POLLIN | POLLPRI;
const int Channel::kWriteEvent=POLLOUT;
const int Channel::kEdgeTrigger=EPOLLET;

Channel::Channel(EventLoop* loop,int fdArg)
  :loop_(loop),
   fd_(fdArg),
   events_(0),
   revents_(0),
   index_(-1),
   tied_(false),
   eventHandling_(false),
   addedToLoop_(false)
{
}



Channel::~Channel()
{
  // 保证事件处理期间Channel不会被析构,Channel的安全措施
  assert(!eventHandling_);
  assert(!addedToLoop_);
  if (loop_->isInLoopThread())
  {
    assert(!loop_->hasChannel(this));
  }
}

// 最后会调用Poller::updateChannel
void Channel::update()
{
  addedToLoop_=true;
  loop_->updateChannel(this);
}

void Channel::remove()
{
  assert(isNoneEvent());
  addedToLoop_ = false;
  loop_->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)
{
  std::shared_ptr<void> guard;
  if (tied_)
  {
    guard = tie_.lock();
    if (guard)
    {
      handleEventWithGuard(receiveTime);
    }
  }
  else
  {
    handleEventWithGuard(receiveTime);
  }
}


void Channel::handleEventWithGuard(Timestamp time)
{
  eventHandling_ = true;
  if(revents_ & POLLNVAL){
    LOG_WARN<<"Channel::handleEvent() POLLNVAL";
  }


  // 两端连接都已经关闭
  if ((revents_ & POLLHUP) && !(revents_ & POLLIN)) {
    LOG_WARN << "Channel::handle_event() POLLHUP";
    if (closeCallback_) closeCallback_();
  }

  if(revents_ & (POLLERR|POLLNVAL)){
    if(errorCallback_) errorCallback_();
  }

  if(revents_ & (POLLIN|POLLPRI|POLLRDHUP)){
    if(readCallback_) readCallback_(time);
  }

  if(revents_ & POLLOUT){
    if(writeCallback_) writeCallback_();
  }
  eventHandling_=false;
}


string Channel::reventsToString() const
{
  return eventsToString(fd_, revents_);
}

string Channel::eventsToString() const
{
  return eventsToString(fd_, events_);
}

string Channel::eventsToString(int fd, int ev)
{
  std::ostringstream oss;
  oss << fd << ": ";
  if (ev & POLLIN)
    oss << "IN ";
  if (ev & POLLPRI)
    oss << "PRI ";
  if (ev & POLLOUT)
    oss << "OUT ";
  if (ev & POLLHUP)
    oss << "HUP ";
  if (ev & POLLRDHUP)
    oss << "RDHUP ";
  if (ev & POLLERR)
    oss << "ERR ";
  if (ev & POLLNVAL)
    oss << "NVAL ";

  return oss.str();
}
