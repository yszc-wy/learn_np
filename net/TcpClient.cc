/*
* Author:           yszc-wy@foxmail.com 
* Encoding:			    utf-8
* Description:      
*/

// 本类头文件
#include "TcpClient.h"
// C系统库头文件

// C++系统库头文件

// 第三方库头文件
#include "base/Logging.h"
// 本项目头文件
#include "Connector.h"
#include "EventLoop.h"
#include "SocketsOps.h"


using namespace yszc;
using namespace yszc::net;
using namespace std::placeholders;


namespace yszc
{
namespace net
{
namespace detail
{

// 在TcpClient被析构之前要将该函数绑定到TcpConnection的closecallback中
void removeConnection(EventLoop* loop, const TcpConnectionPtr& conn)
{
  // 对channel的操作一定要移动到channel循环的外侧
  loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}

void removeConnector(const ConnectorPtr& connector)
{
  //connector->
}

}  // namespace detail
}  // namespace net
}  // namespacea yszc


TcpClient::TcpClient(EventLoop* loop,
          const InetAddress& serverAddr,
          const string& nameArg)
  :loop_(CHECK_NOTNULL(loop)),
   connector_(new Connector(loop_,serverAddr)),
   connectionCallback_(defaultConnectionCallback),
   messageCallback_(defaultMessageCallback),
   name_(nameArg),
   retry_(false),
   connect_(true)
{
  connector_->setNewConnectionCallback(
      std::bind(&TcpClient::newConnection, this, _1));
  // FIXME setConnectFailedCallback
  LOG_INFO << "TcpClient::TcpClient[" << name_
           << "] - connector " << connector_.get();
}


TcpClient::~TcpClient()
{
  LOG_INFO << "TcpClient::~TcpClient[" << name_
           << "] - connector " << connector_.get();
  TcpConnectionPtr conn;
  bool unique = false;
  {
    MutexLockGuard lock(mutex_);
    unique = connection_.unique();
    conn = connection_;
  }
  // tcpclient不负责管理tcpconnection的生命周期,因此tcpclient的析构会造成tcpconnection无法调用已经注册好的closecallback
  // 因此需要修改closecallback来保证tcpclient在析构后tcpconnection仍然可以正常的销毁
  if (conn)
  {
    // 修改tcpconnection原有的closecallback
    assert(loop_ == conn->getLoop());
    // FIXME: not 100% safe, if we are in different thread
    CloseCallback cb = std::bind(&detail::removeConnection, loop_, _1);
    loop_->runInLoop(
        std::bind(&TcpConnection::setCloseCallback, conn, cb));
    // 如果conn只被tcpclient引用,意味着tcpclient的销毁会导致tcpconneciton的销毁(无论tcpconnection是否结束或正在运行,为了防止tcpconnection在断开连接前析构所导致的问题,我们为tcpconnection编写强制销毁操作)
    if (unique)
    {
      // 需要处理正在传输的数据等...
      conn->forceClose();
    }
  }
  else
  {
    // 如果conn为空,有可能还在等待connector回调newConnectionCallback,此时connector的引用计数如果为1,tcpclient的析构会导致connector析构
    // connector析构后,其注册的channel会在回调时访问到空指针
    connector_->stop();
    // FIXME: HACK
    // 为了防止该情况发生,我们runafter延长channel的生命周期,期待在这个时间段内可以释放channel
    loop_->runAfter(1, std::bind(&detail::removeConnector, connector_));
  }
}


void TcpClient::connect()
{
  // FIXME: check state
  LOG_INFO << "TcpClient::connect[" << name_ << "] - connecting to "
           << connector_->serverAddress().toHostPort();
  connect_ = true;
  connector_->start();
}

void TcpClient::disconnect()
{
  connect_ = false;

  {
    MutexLockGuard lock(mutex_);
    if (connection_)
    {
      connection_->shutdown();
    }
  }
}

void TcpClient::stop()
{
  connect_ = false;
  connector_->stop();
}


// in loop,不会被外部线程调用
void TcpClient::newConnection(int sockfd)
{
  loop_->assertInLoopThread();
  InetAddress peerAddr(sockets::getPeerAddr(sockfd));
  char buf[32];
  // ipport+connectid
  snprintf(buf, sizeof buf, ":%s#%d", peerAddr.toHostPort().c_str(), nextConnId_);
  ++nextConnId_;
  string connName = name_ + buf;

  InetAddress localAddr(sockets::getLocalAddr(sockfd));
  // FIXME poll with zero timeout to double confirm the new connection
  // FIXME use make_shared if necessary
  TcpConnectionPtr conn(new TcpConnection(loop_,
                                          connName,
                                          sockfd,
                                          localAddr,
                                          peerAddr));

  conn->setConnectionCallback(connectionCallback_);
  conn->setMessageCallback(messageCallback_);
  conn->setWriteCompleteCallback(writeCompleteCallback_);
  conn->setCloseCallback(
      std::bind(&TcpClient::removeConnection, this, _1)); // FIXME: unsafe
  // 保证线程安全,防止外部线程在使用connection时connection突然改变
  {
    MutexLockGuard lock(mutex_);
    connection_ = conn;
  }
  conn->connectEstablished();
}
void TcpClient::removeConnection(const TcpConnectionPtr& conn)
{
  loop_->assertInLoopThread();
  assert(loop_ == conn->getLoop());

  // 先保证connection与conn相同,再清除client的connection_
  {
    MutexLockGuard lock(mutex_);
    assert(connection_ == conn);
    connection_.reset();
  }

  // 和tcpserver的情况一样,此时tcpconnection同样快被析构了,需要使用bind延长生命周期并在dopendfun进行安全的删除操作
  loop_->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
  // 以上操作完整清除了原有的tcpconnection

  // 如果开启了断开重连模式,就会自动启动重连操作
  if (retry_ && connect_)
  {
    LOG_INFO << "TcpClient::connect[" << name_ << "] - Reconnecting to "
             << connector_->serverAddress().toHostPort();
    connector_->restart();
  }
}
