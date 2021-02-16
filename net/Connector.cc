/*
* Author:           yszc-wy@foxmail.com 
* Encoding:			    utf-8
* Description:      
*/

// 本类头文件
#include "Connector.h"
// C系统库头文件

// C++系统库头文件

// 第三方库头文件
// 本项目头文件
#include "base/Logging.h"
#include "Channel.h"
#include "EventLoop.h"
#include "SocketsOps.h"

using namespace yszc;
using namespace yszc::net;

const int Connector::kMaxRetryDelayMs;



Connector::Connector(EventLoop* loop, const InetAddress& serverAddr)
  : loop_(loop),
    serverAddr_(serverAddr),
    connect_(false),
    state_(kDisconnected),
    retryDelayMs_(kInitRetryDelayMs)
{
  LOG_DEBUG << "ctor[" << this << "]";
}


Connector::~Connector()
{
  LOG_DEBUG << "dtor[" << this << "]";
  // 为了防止Connector在loop_中定时器到期前析构,Connector采用析构时取消定时器的操作
  loop_->cancel(timerId_);
  assert(!channel_);
}


void Connector::start()
{
  connect_ = true;
  loop_->runInLoop(std::bind(&Connector::startInLoop, this)); // FIXME: unsafe,传递的是this指针,若Connector先一步析构,可能导致loop中的connector访问到悬空指针
}


void Connector::startInLoop()
{
  loop_->assertInLoopThread();
  assert(state_ == kDisconnected);
  if (connect_)
  {
    connect();
  }
  else
  {
    LOG_DEBUG << "do not connect";
  }
}


void Connector::connect()
{
  int sockfd = sockets::createNonblockingOrDie();
  int ret = sockets::connect(sockfd, serverAddr_.getSockAddr());
  int savedErrno = (ret == 0) ? 0 : errno;
  switch (savedErrno)
  {
    // 正在连接,需要注册channel来等待连接完成(可写监听)
    case 0:
    case EINPROGRESS:
    case EINTR:
    case EISCONN:
      connecting(sockfd);
      break;

    case EAGAIN:
    case EADDRINUSE:
    case EADDRNOTAVAIL:
    case ECONNREFUSED:
    case ENETUNREACH:
      // 出现错误重试
      retry(sockfd);
      break;

    case EACCES:
    case EPERM:
    case EAFNOSUPPORT:
    case EALREADY:
    case EBADF:
    case EFAULT:
    case ENOTSOCK:
      // 其他错误就立即关闭
      LOG_SYSERR << "connect error in Connector::startInLoop " << savedErrno;
      sockets::close(sockfd);
      break;

    default:
      LOG_SYSERR << "Unexpected error in Connector::startInLoop " << savedErrno;
      sockets::close(sockfd);
      // connectErrorCallback_();
      break;
  }
}

void Connector::connecting(int sockfd)
{
  setState(kConnecting);
  assert(!channel_);
  // 从这里开始注册channel
  channel_.reset(new Channel(loop_, sockfd));
  channel_->setWriteCallback(
      std::bind(&Connector::handleWrite, this)); // FIXME: unsafe
  channel_->setErrorCallback(
      std::bind(&Connector::handleError, this)); // FIXME: unsafe

  // channel_->tie(shared_from_this()); is not working,
  // as channel_ is not managed by shared_ptr
  channel_->enableWriting();
}

void Connector::handleWrite()
{
  LOG_TRACE << "Connector::handleWrite " << state_;

  if (state_ == kConnecting)
  {
    // 当连接完毕,此时我们不再需要channel,这是需要删除并重置channel
    int sockfd = removeAndResetChannel();
    // 进一步确认是否连接成功
    int err = sockets::getSocketError(sockfd);
    if (err)
    {
      LOG_WARN << "Connector::handleWrite - SO_ERROR = "
               << err << " " << strerror_tl(err);
      // 如果出现错误就重试
      retry(sockfd);
    }
    // 如果先启动客户端,且目标地址是本机的话,为客户端随机选取的源地址和端口有可能恰巧就是目标地址与端口,这就发生了自连接
    else if (sockets::isSelfConnect(sockfd))
    {
      LOG_WARN << "Connector::handleWrite - Self connect";
      retry(sockfd);
    }
    else
    {
      setState(kConnected);
      if (connect_)
      {
        newConnectionCallback_(sockfd);
      }
      else
      {
        sockets::close(sockfd);
      }
    }
  }
  else
  {
    // what happened?
    assert(state_ == kDisconnected);
  }
}

void Connector::handleError()
{
  LOG_ERROR << "Connector::handleError";
  assert(state_ == kConnecting);
    
  int sockfd = removeAndResetChannel();
  int err = sockets::getSocketError(sockfd);
  LOG_TRACE << "SO_ERROR = " << err << " " << strerror_tl(err);
  retry(sockfd);
}


int Connector::removeAndResetChannel()
{
  channel_->disableAll();
  // 新接口
  channel_->remove();
  // 在外部使用时,我们只使用channel的接口
  // loop_->removeChannel(channel_.get());
  int sockfd = channel_->fd();
  // 不能在这里reset channel_,因为此时我们处于Channel::handleEvent,所以我们把他推迟到任务queue中
  loop_->queueInLoop(std::bind(&Connector::resetChannel, this)); // FIXME: unsafe 这里Connector可能先于loop_析构
  return sockfd;
}


void Connector::resetChannel()
{
  channel_.reset();
}

// 重试就是关闭当前已经失败的sockfd
void Connector::retry(int sockfd)
{
  sockets::close(sockfd);
  setState(kDisconnected);
  // 表示之前已经调用start,正在进行连接
  if (connect_)
  {
    LOG_INFO << "Connector::retry - Retry connecting to "
             << serverAddr_.toHostPort() << " in "
             << retryDelayMs_ << " milliseconds. ";
    timerId_ = loop_->runAfter(retryDelayMs_/1000.0,  // FIXME: unsafe
                               std::bind(&Connector::startInLoop, this));
    retryDelayMs_ = std::min(retryDelayMs_ * 2, kMaxRetryDelayMs);
  }
  else
  {
    LOG_DEBUG << "do not connect";
  }
}


void Connector::restart()
{
  loop_->assertInLoopThread();
  setState(kDisconnected);
  retryDelayMs_ = kInitRetryDelayMs;
  connect_ = true;
  startInLoop();
}

void Connector::stop()
{
  connect_ = false;
  loop_->cancel(timerId_);
  assert(!channel_);
}
