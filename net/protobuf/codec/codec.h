/*
* Author:           yszc-wy@foxmail.com 
* Encoding:			    utf-8
* Description:      负责分包、检查检验和并解析出message对象,使用messge对象回调dispatcher的函数,错误处理占了很大一部分代码量
*/

// 包结构
// struct ProtobufTransportFormat __attribute__ ((__packed__))
// {
//   int32_t  len; // 表示下面数据的全长
//   int32_t  nameLen; // 表示typeName的长度(包含\0)
//   char     typeName[nameLen];
//   char     protobufData[len-nameLen-8];
//   int32_t  checkSum; // adler32 of nameLen, typeName and protobufData
// }


#pragma once

// C系统库头文件

// C++系统库头文件
#include <string>
#include <functional>
#include <memory>
// 第三方库头文件
#include <google/protobuf/message.h>
// 本项目头文件
#include "base/Timestamp.h"
#include "net/TcpConnection.h"



// 系统结构体或其他命名空间中类的前向声明(注意要包含相应的命名空间)




namespace yszc
{

// 本命名空间内部类的前向声明
namespace net
{
// codec的过程中要生成message对象,我们使用shared_ptr来管理
typedef std::shared_ptr<google::protobuf::Message> MessagePtr;
class ProtobufCodec 
{
 public:

  // 记录解码过程中产生的错误
  // 每次分包和解析出现错误后会把ErrorCode传递给errormessagecallback
  enum ErrorCode
  {
    kNoError = 0, // 未出现错误
    kInvalidLength, // 非法长度
    kCheckSumError, // 校验和错误
    kInvalidNameLen,  
    kUnknownMessageType,
    kParseError,
  };
  // 配套的errormessagecallback

  typedef std::function<void (const yszc::net::TcpConnectionPtr&,
                                const MessagePtr&,
                                Timestamp)> ProtobufMessageCallback;

  typedef std::function<void (const yszc::net::TcpConnectionPtr&,
                                yszc::net::Buffer*,
                                Timestamp,
                                ErrorCode)> ErrorCallback;

  ProtobufCodec(ProtobufMessageCallback protobufMessageCallback):
    messageCallback_(protobufMessageCallback),
    errorCallback_(defaultErrorCallback)
  {
  }

  // 接受原始包,解析出message
  void onMessage(const yszc::net::TcpConnectionPtr& conn,
                 yszc::net::Buffer* buf,
                 Timestamp receiveTime);


  // 接受message,包装成字节流并发送
  void send(const yszc::net::TcpConnectionPtr& conn,
            const google::protobuf::Message& message)
  {
    // FIXME: serialize to TcpConnection::outputBuffer()
    yszc::net::Buffer buf;
    fillEmptyBuffer(&buf, message);
    conn->send(&buf);
  }


  static void fillEmptyBuffer(Buffer* buf, const google::protobuf::Message& message);
  static const std::string& errorCodeToString(ErrorCode errorCode);
  // message解析函数
  static google::protobuf::Message* createMessage(const std::string& typeName);
 private:
  // 解析len之后的字段,检查校验和
  static MessagePtr parse(const char* buf, int len, ErrorCode* error);
  // 自带的错误处理函数
  static void defaultErrorCallback(const yszc::net::TcpConnectionPtr& conn,
                                  yszc::net::Buffer* buf,
                                  Timestamp,
                                  ErrorCode errorCode); // 额外包含一个errorCode
  

  ProtobufMessageCallback messageCallback_;
  ErrorCallback errorCallback_;
  const static int kHeaderLen=sizeof(int32_t);
  const static int kMinMessageLen=2*sizeof(int32_t)+2;// namelen+checksum+\0+1 byte protobuf data
  const static int kMaxMessageLen=64*1024*1024;// 最大包长度64mb
};

}

}

