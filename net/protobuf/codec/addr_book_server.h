/*
* Author:           yszc-wy@foxmail.com 
* Encoding:			    utf-8
* Description:      
*/
#pragma once

// C系统库头文件

// C++系统库头文件

// 第三方库头文件
#include <boost/noncopyable.hpp>
// 本项目头文件
#include "base/Timestamp.h"
#include "base/Logging.h"
#include "net/TcpServer.h"
#include "codec.h"
#include "dispatcher.h"
#include "addr_book.pb.h"



// 系统结构体或其他命名空间中类的前向声明(注意要包含相应的命名空间)
typedef std::shared_ptr<google::protobuf::Message> MessagePtr;
typedef std::shared_ptr<tutorial::Person> PersonPtr;
typedef std::shared_ptr<tutorial::Person_PhoneNumber> PhoneNumberPtr;
typedef std::shared_ptr<tutorial::AddressBook> AddrBookPtr;

using namespace yszc;
using namespace yszc::net;

class AddrBookServer
{
 public:  
  AddrBookServer(yszc::EventLoop* loop,
                 const yszc::net::InetAddress& listenAddr);

  void start()
  {
    server_.start();
  }

  void onConnection(const yszc::net::TcpConnectionPtr& conn);


  void onUnknownMessage(const yszc::net::TcpConnectionPtr& conn,
                        const MessagePtr& message,
                        Timestamp receiveTime);

  void onPerson(const yszc::net::TcpConnectionPtr& conn,
                const PersonPtr& person,
                Timestamp receiveTime);

  void onPhoneNumber(const yszc::net::TcpConnectionPtr& conn,
                const PhoneNumberPtr& phone_number,
                Timestamp receiveTime);

  void onAddressBook(const yszc::net::TcpConnectionPtr& conn,
                const AddrBookPtr& address_book,
                Timestamp receiveTime);

 private:
  yszc::net::TcpServer server_;
  yszc::net::ProtobufDispatcher dispatcher_;  
  yszc::net::ProtobufCodec codec_;
};


