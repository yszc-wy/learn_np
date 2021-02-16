/*
* Author:           yszc-wy@foxmail.com 
* Encoding:			    utf-8
* Description:      
*/

// 本类头文件

// C系统库头文件

// C++系统库头文件

// 第三方库头文件

// 本项目头文件


#include "net/Poller.h"
#include "net/poller/PollPoller.h"
#include "net/poller/EPollPoller.h"

#include <stdlib.h>

using namespace yszc::net;

Poller* Poller::newDefaultPoller(EventLoop* loop)
{
  if (::getenv("WEB_USE_POLL"))
  {
    return new PollPoller(loop);
  }
  else
  {
    return new EPollPoller(loop);
  }
}

