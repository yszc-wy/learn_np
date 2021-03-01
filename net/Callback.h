/*
* Author:           
* Encoding:			    utf-8
* Description:      所有客户端可见的callback类型注册
*/

#include <memory>
#include <functional>
#include "base/Timestamp.h"
namespace yszc{
typedef std::function<void()> TimerCallback;
namespace net{

class Buffer;
class TcpConnection;

typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
typedef std::weak_ptr<TcpConnection> WeakTcpConnectionPtr;
}

typedef std::function<void()> TimerCallback;
typedef std::function<void (const yszc::net::TcpConnectionPtr&)> ConnectionCallback;
typedef std::function<void (const yszc::net::TcpConnectionPtr&,
                              net::Buffer* buf,
                              Timestamp)> MessageCallback;
typedef std::function<void (const yszc::net::TcpConnectionPtr&)> WriteCompleteCallback;
typedef std::function<void (const yszc::net::TcpConnectionPtr&)> CloseCallback;
typedef std::function<void (const yszc::net::TcpConnectionPtr&, size_t)> HighWaterMarkCallback;
}

