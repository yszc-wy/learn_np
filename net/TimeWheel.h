/*
* Author:           
* Encoding:			    utf-8
* Description:      
*/
#pragma once

// C系统库头文件

// C++系统库头文件
#include <memory>
#include <unordered_set>
// 第三方库头文件
#include <boost/circular_buffer.hpp>
// 本项目头文件
#include "net/Callback.h"


namespace yszc
{

namespace net
{

class TcpConnection;

class TimeWheel
{
 public:
  // TimeWheel 踢出空闲链接
  class Entry
  {
   public:
    Entry(const WeakTcpConnectionPtr& weakptr)
      : weakptr_(weakptr)
    {
    }
    ~Entry();
   private:
    // 不想因为Entry的存在控制了Tcpconnection的生命周期,所以使用弱引用
    WeakTcpConnectionPtr weakptr_;
  };
  // 当EntryPtr的引用计数减少到0会自动触发析构操作,断开连接
  typedef std::shared_ptr<Entry> EntryPtr;
  // Tcpconnection持有,TcpConnection不应该持有EntryPtr导致延长Entry的生命周期
  typedef std::weak_ptr<Entry> WeakEntryPtr;
  static const int kTimeGap=8;
  typedef std::unordered_set<EntryPtr> Bucket;
  typedef boost::circular_buffer<Bucket> BucketList;
};

}


}

