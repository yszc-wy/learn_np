/*
* Author:           
* Encoding:			    utf-8
* Description:      
*/

// 本类头文件
#include "Acceptor.h"
// C系统库头文件
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
// C++系统库头文件
#include <iostream>
// 第三方库头文件

// 本项目头文件
#include "base/Logging.h"
#include "SocketsOps.h"
#include "EventLoop.h"
#include "InetAddress.h"



using namespace yszc;
using namespace yszc::net;

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr,bool reusePort)
  :loop_(loop),
   acceptSocket_(sockets::createNonblockingOrDie()),
   acceptChannel_(loop,acceptSocket_.fd()),
   listenning_(false),
   idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))
{
  assert(idleFd_ >= 0);
  acceptSocket_.setReuseAddr(true);
  // 绑定地址,但不监听
  acceptSocket_.bindAddress(listenAddr);
  acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead,this));
  // 注意在这里不要enableReading
}


Acceptor::~Acceptor()
{
  acceptChannel_.disableAll();
  acceptChannel_.remove();
  ::close(idleFd_);
}

void Acceptor::listen(){
  // 保证在loop线程运行
  loop_->assertInLoopThread();
  acceptSocket_.listen();
  listenning_=true;
  // 在这里才打开事件监听
  acceptChannel_.enableReading();
  // acceptChannel_.enableET();
}


// void Acceptor::handleRead(){
  // loop_->assertInLoopThread();
  // net::InetAddress peerAddr(0);
  // // 这里存在描述符资源分配的情况,会有描述符耗尽的可能
  // int connfd=acceptSocket_.accept(&peerAddr);
  // if(connfd>=0){
    // if(newConnectionCallback_){
      // // 暴露newConnectionCallback回调接口
      // newConnectionCallback_(connfd,peerAddr);
    // } else{
      // // 如果没有处理函数,就直接断开连接
      // sockets::close(connfd);
    // }
  // } else{
    // // 防止文件描述符耗尽造成busy loop
    // LOG_SYSERR << "in Acceptor::handleRead";
    // // Read the section named "The special problem of
    // // accept()ing when you can't" in libev's doc.
    // // By Marc Lehmann, author of libev.
    // if (errno == EMFILE)
    // {
      // ::close(idleFd_);
      // idleFd_ = ::accept(acceptSocket_.fd(), NULL, NULL);
      // ::close(idleFd_);
      // idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
    // }
  // }
// }

// accept直到为空,这会导致连接稀少时会多accept一次,没啥大不了的
void Acceptor::handleRead(){
  loop_->assertInLoopThread();
  net::InetAddress peerAddr(0);
  // 这里存在描述符资源分配的情况,会有描述符耗尽的可能
  int connfd;
  
  int dup=0;
  while((connfd=acceptSocket_.accept(&peerAddr))>=0)
  {
    if(connfd>=65535){
      LOG_WARN << "Open too many fd in Acceptor";
      ::close(connfd);
      continue;
    }

    if(newConnectionCallback_){
      // 暴露newConnectionCallback回调接口
      newConnectionCallback_(connfd,peerAddr);
    } else{
      // 如果没有处理函数,就直接断开连接
      sockets::close(connfd);
    }
    ++dup;
  }

  if(errno==EMFILE)
  {
    LOG_FATAL<<"Open too many fd in Acceptor (hard limit)";
  }
  // if(dup!=1)
    // std::cout<<"\n dup accept! "<<dup<<std::endl;

  // if(errno==EMFILE)
  // {
    // ::close(idleFd_);
    // idleFd_ = ::accept(acceptSocket_.fd(), NULL, NULL);
    // ::close(idleFd_);
    // idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
  // }
  
}
