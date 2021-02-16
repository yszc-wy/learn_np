/*
* Author:           yszc-wy@foxmail.com 
* Encoding:			    utf-8
* Description:      Channel主要负责一个fd的dispatcher,enableReading中虽然有修改poller类fd的功能,但这是借助了EventLoop提供的接口实现的,不是该类的功能(属于Channel对象与Poller对象之间的联系,而非Channel对象本身的功能) Channel的生命周期是如何管理的
* 存在问题:
* 在loop中执行析构channel的操作有可能会吧activate channel中的某个channelz指针变为悬空指针
* 如何处理连接断开后,TcpConnection 中Channel对象的析构和Poller对象中Channel对象的删除,如何防止Channel在handleEvent closeCallback时被delete造成程序崩溃
* 解:在handleClose中将poller中的event设置为NONE(取消监听,并remove_channel)
* 如何保证eventloop和poller不会访问到已经析构的channel(个人想法是在closecallback中断开与poller的联系,然后安全的析构Channel)
* 如何保证TcpServer在析构时,其所持有的Channel可以从Poller和loop中安全的删除
* 要监听的fd,要监听的事件events,以及事件触发之后需要执行的函数callback
* revents表示发生了哪个类型的事件
*/

#pragma once
#include <functional>
#include <memory>
#include "base/noncopyable.h"
#include "base/Timestamp.h"

namespace yszc{

namespace net
{

class EventLoop;

// 只在io线程内部调用,不必考虑线程安全
class Channel: noncopyable
{
 public:
  typedef std::function<void()> EventCallback;
  // 由于tcpconnection的message回调需要poller解阻塞的时间,所以特意在channel中提供该回调,并修改channel handleevent和eventloop的loop接口
  typedef std::function<void(Timestamp)> ReadEventCallback;
  // 一个Channel只属于一个Eventloop,
  Channel(EventLoop* loop,int fd);
  ~Channel();
  void handleEvent(Timestamp receiveTime);
  void setReadCallback(const ReadEventCallback& cb)
  { readCallback_=cb;}
  void setWriteCallback(const EventCallback& cb)
  { writeCallback_=cb;}
  void setErrorCallback(const EventCallback& cb)
  { errorCallback_=cb;}
  void setCloseCallback(const EventCallback& cb)
  { closeCallback_=cb;}


  /// Tie this channel to the owner object managed by shared_ptr,
  /// prevent the owner object being destroyed in handleEvent.
  void tie(const std::shared_ptr<void>&);

  // 接口函数
  int fd() const {return fd_;}
  int events() const {return events_;}
  void set_revents(int revt){revents_=revt;}
  bool isNoneEvent() const {return events_==kNoneEvent;}

  void enableReading() {events_|=kReadEvent;update();}
  // 从poller中去除该channel的event标识
  void disableAll() { events_ = kNoneEvent; update(); }
  // 电平触发:打开后要及时关闭,防止busyloop
  void enableWriting() { events_|=kWriteEvent;update(); }
  void disableWriting() { events_&=~kWriteEvent;update(); }
  bool isWriting() const { return events_ & kWriteEvent; }
  
  

  // for Poller
  int index() {return index_;}
  void set_index(int idx){index_=idx;}


  // for debug
  string reventsToString() const;
  string eventsToString() const;

  EventLoop* ownerLoop() {return loop_;}
  void remove();
  

 private:
  void handleEventWithGuard(Timestamp receiveTime);
  static string eventsToString(int fd, int ev);

  void update();
  static const int kNoneEvent;
  static const int kReadEvent;
  static const int kWriteEvent;

  EventLoop* loop_;
  // pollfd中的参数
  const int fd_;
  // 下面2个参数可以用k*Event来填充,代表想要检查的事件
  int events_;
  int revents_;
  // index_标识当前Channel在Poller中的状态,是未加入,已加入、还是已删除
  // 使用index_的目的是在updateChannel是Poller不必对Channelset进行检索
  // 直接检查index的标志就直到该channel是否插入到Poller中
  // 观察者可以使用的技巧
  int index_; // used by Poller

  // 将channel与某个对象绑定,在事件回调时时如果该对象销毁了,就不会执行handleEventGurad
  std::weak_ptr<void> tie_;
  bool tied_;
  // 标识handleEvent是否正在运行
  bool eventHandling_;
  // 标识该channel是否已经加入Loop
  bool addedToLoop_;

  ReadEventCallback readCallback_;
  EventCallback writeCallback_;
  EventCallback errorCallback_;
  EventCallback closeCallback_;
};


}
}
