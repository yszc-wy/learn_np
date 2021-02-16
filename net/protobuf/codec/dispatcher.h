/*
* Author:           yszc-wy@foxmail.com 
* Encoding:			    utf-8
* Description:      
*/
#pragma once

// C系统库头文件

// C++系统库头文件
#include <functional>
#include <map>
// 第三方库头文件
#include <boost/noncopyable.hpp>
#include <google/protobuf/message.h>
// 本项目头文件
#include "base/Timestamp.h"
#include "net/TcpConnection.h"



// 系统结构体或其他命名空间中类的前向声明(注意要包含相应的命名空间)


namespace yszc
{

// 本命名空间内部类的前向声明
typedef std::shared_ptr<google::protobuf::Message> MessagePtr;
namespace net{

class Callback : public boost::noncopyable{
 public:
  virtual ~Callback()=default;
  virtual void onMessage(const yszc::net::TcpConnectionPtr&,
                         const MessagePtr&,
                         Timestamp) const =0;
};

template <typename T>
class CallbackT: public Callback{
 public: 
  // T必须是Message的子类
  static_assert(std::is_base_of<google::protobuf::Message, T>::value,
                "T must be derived from gpb::Message.");
  typedef std::function<void (const yszc::net::TcpConnectionPtr&,
                                const std::shared_ptr<T>&,
                                Timestamp)> ProtobufMessageTCallback;
  CallbackT(ProtobufMessageTCallback callback):
    callback_(callback)
  {
  }
  
  void onMessage(const yszc::net::TcpConnectionPtr& conn,
                         const MessagePtr& message,
                         Timestamp receiveTime) const override
  {
    std::shared_ptr<T> concrete = std::static_pointer_cast<T>(message);
    assert(concrete != NULL);
    callback_(conn, concrete, receiveTime);
  };
  
  ProtobufMessageTCallback callback_;
};

class ProtobufDispatcher{
 public:
  typedef std::function<void (const yszc::net::TcpConnectionPtr&,
                                const MessagePtr& message,
                                Timestamp)> ProtobufMessageCallback;

  explicit ProtobufDispatcher(const ProtobufMessageCallback& defaultCb)
    : defaultCallback_(defaultCb)
  {
  }

  void onProtobufMessage(const yszc::net::TcpConnectionPtr& conn,
                         const MessagePtr& message,
                         Timestamp receiveTime) const
  {
    CallbackMap::const_iterator it = callbacks_.find(message->GetDescriptor());
    if (it != callbacks_.end())
    {
      it->second->onMessage(conn, message, receiveTime);
    }
    else
    {
      defaultCallback_(conn, message, receiveTime);
    }
  }

  template<typename T>
  void registerMessageCallback(const typename CallbackT<T>::ProtobufMessageTCallback& callback)
  {
    std::shared_ptr<CallbackT<T> > pd(new CallbackT<T>(callback));
    callbacks_[T::descriptor()] = pd;
  }
 private:
  typedef std::map<const google::protobuf::Descriptor*, std::shared_ptr<Callback> > CallbackMap;

  CallbackMap callbacks_;
  ProtobufMessageCallback defaultCallback_;
};

}


}

