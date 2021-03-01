/*
* Encoding:			    utf-8
* Description:      
*/

// 本类头文件

// C系统库头文件

// C++系统库头文件
#include <vector>
#include <list>
#include <set>
#include <unordered_set>
// 第三方库头文件
#include <boost/circular_buffer.hpp>
// 本项目头文件
#include "net/EventLoop.h"
#include "net/TcpServer.h"
#include "net/TcpConnection.h"

using namespace yszc;
using namespace yszc::net;

class WheelServer
{
 public:
  WheelServer(EventLoop* loop,const InetAddress& addr)
    : loop_(loop),
      server_(loop_,addr),
      list_(kTimeGap),
  {
    loop_->runEvery(1,std::bind(&WheelServer::timeCallback,this));
  }
  void timeCallback()
  {
    list_.push_back(Bucket());
  }
  void onConnection(const TcpConnectionPtr& conn)
  {
    if(conn->connected())
    {
      EntryPtr entry_ptr(new Entry(conn));
      list_.back().insert(entry_ptr);
      WeakEntryPtr weak_entry_ptr(entry_ptr);
      // tcpconnection必须持有entry的弱指针,如果持有强引用会导致Entry无法析构,也就无法调用析构函数断开连接
      conn->setContext(entry_ptr);
    }  
  }
  void onMessage(const TcpConnectionPtr& conn,
                 Buffer* buf,
                 Timestamp receiveTime)
  {
    // 就算调用了shutdown,这里依然可以接受到数据,只不过无法发送
    // 不必将原来位置的conn删除,因为movePoint移动到该位置会自动将其删除
    WeakEntryPtr weak_ptr(boost::any_cast<WeakEntryPtr>(conn->getContext()));
    EntryPtr ptr=weak_ptr.lock();
    if(ptr)
    {
      // 这里每次收到消息都要插入,存在效率问题
      list_.back().insert(ptr); 
    }
  }
 private:
  typedef std::weak_ptr<TcpConnection> WeakTcpConnectionPtr;
  class Entry
  {
   public:
    Entry(const WeakTcpConnectionPtr& weakptr)
      : weakptr_(weakptr)
    {
    }
    ~Entry()
    {
      TcpConnectionPtr ptr=weakptr_.lock();
      if(ptr){
        ptr->shutdown();
      }
    }
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
  
  EventLoop *loop_;
  TcpServer server_;
  BucketList list_;
};

