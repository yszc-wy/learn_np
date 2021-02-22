/*
* Author:           yszc-wy@foxmail.com 
* Encoding:			    utf-8
* Description:      
*/
#include "net/http/HttpServer.h"

#include "base/Logging.h"
#include "net/EventLoop.h"
#include "net/TcpConnection.h"
#include "net/http/HttpContext.h"
#include "net/http/HttpRequest.h"
#include "net/http/HttpResponse.h"


using namespace yszc;
using namespace yszc::net;
using namespace std::placeholders;

namespace yszc
{
namespace net
{
namespace detail
{

void defaultHttpCallback(const HttpRequest&, HttpResponse* resp)
{
  resp->setStatusCode(HttpResponse::k404NotFound);
  resp->setStatusMessage("Not Found");
  resp->setCloseConnection(true);
}

}  // namespace detail
}  // namespace net
}  // namespace yszc

HttpServer::HttpServer(EventLoop* loop,
                       const InetAddress& listenAddr,
                       const string& name,
                       TcpServer::Option option)
  : server_(loop, listenAddr, name, option),
    httpCallback_(detail::defaultHttpCallback),
    list_(8)
{
  server_.setConnectionCallback(
      std::bind(&HttpServer::onConnection, this, _1));
  server_.setMessageCallback(
      std::bind(&HttpServer::onMessage, this, _1, _2, _3));

  // 注册时间回调
  server_.getLoop()->runEvery(1,std::bind(&HttpServer::onTimer,this));
}

void HttpServer::start()
{
  LOG_WARN << "HttpServer[" << server_.name()
    << "] starts listening on " << server_.ipPort();
  server_.start();
}

void HttpServer::onConnection(const TcpConnectionPtr& conn)
{
  if (conn->connected())
  {
    TimeWheel::BucketList::iterator tail;
    TimeWheel::EntryPtr entry_ptr(new TimeWheel::Entry(conn));
    {
      MutexLockGuard lock(mutex_);
      list_.back().insert(entry_ptr);
      tail=list_.end()-1;
    }
    TimeWheel::WeakEntryPtr weak_entry_ptr(entry_ptr);
    // tcpconnection必须持有entry的弱指针,如果持有强引用会导致Entry无法析构,也就无法调用析构函数断开连接
    conn->setTimeWheelWeakEntry(weak_entry_ptr);
    conn->setTimeWheelTail(tail);
    conn->setContext(HttpContext());
  }
}

void HttpServer::onMessage(const TcpConnectionPtr& conn,
                           Buffer* buf,
                           Timestamp receiveTime)
{
  // 不必将原来位置的conn删除,因为movePoint移动到该位置会自动将其删除
  TimeWheel::WeakEntryPtr weak_ptr(conn->getTimeWheelWeakEntry());
  TimeWheel::EntryPtr ptr=weak_ptr.lock();
  if(ptr)
  {
    // 这里每次收到消息都要插入,存在效率问题
    {
      MutexLockGuard lock(mutex_);
      TimeWheel::BucketList::iterator tail=list_.end()-1;
      if(tail!=conn->getTimeWheelTail())
        list_.back().insert(ptr); 
    }
  }


  HttpContext* context = boost::any_cast<HttpContext>(conn->getMutableContext());

  if (!context->parseRequest(buf, receiveTime))
  {
    conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
    conn->shutdown();
  }

  // 如果已经获取了所有的数据
  if (context->gotAll())
  {
    onRequest(conn, context->request());
    context->reset();
  }
}

void HttpServer::onRequest(const TcpConnectionPtr& conn, const HttpRequest& req)
{
  // 获取Connection字段
  const string& connection = req.getHeader("Connection");
  // 如果是Keepalive
  bool close = connection == "close" ||
    (req.getVersion() == HttpRequest::kHttp10 && connection != "Keep-Alive");
  // 使用连接类型初始化response
  HttpResponse response(close); 
  // 填充response
  httpCallback_(req, &response);
  Buffer buf;
  // 将response装载到buffer中
  response.appendToBuffer(&buf);
  // 发送
  conn->send(&buf);
  // 如果不是Keepalive,就会主动断开连接
  if (response.closeConnection())
  {
    conn->shutdown();
  }
}

