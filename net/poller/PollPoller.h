/*
* Author:           yszc-wy@foxmail.com 
* Encoding:			    utf-8
* Description:      Poller只负责I/O复用,传出active channel,不负责消息分发(handleevent)
* 维护正处于监听状态的Channel表
*/

#pragma once

#include <vector>

#include "net/Poller.h"


struct pollfd;

namespace yszc
{

namespace net
{

// Poller类似发布者,会发布激活的channel
class PollPoller : public Poller
{
 public:

  PollPoller(EventLoop* loop);
  ~PollPoller() override;

  Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;
  void updateChannel(Channel* channel) override;
  void removeChannel(Channel* channel) override;

 private:
  void fillActiveChannels(int numEvents,
                          ChannelList* activeChannels) const;

  typedef std::vector<struct pollfd> PollFdList;
  PollFdList pollfds_;
};

}


}
