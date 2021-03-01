/*
* Encoding:			    utf-8
* Description:      
*/

#include <boost/noncopyable.hpp>
#include <iostream>

#include "base/Logging.h"
#include "dispatcher.h"
#include "codec.h"
#include "addr_book.pb.h"
#include "net/EventLoop.h"
#include "net/TcpClient.h"
using namespace yszc;
using namespace yszc::net;
using namespace std::placeholders;
using namespace std;


google::protobuf::Message* messageToSend;

class AddrBookClient : boost::noncopyable
{
 public:
  AddrBookClient(EventLoop* loop,const InetAddress& serverAddr)
    : loop_(loop),
      client_(loop,serverAddr,"addrbook"),
      dispatcher_(std::bind(&AddrBookClient::onUnknownMessage,this,_1,_2,_3)),
      codec_(std::bind(&ProtobufDispatcher::onProtobufMessage,&dispatcher_,_1,_2,_3))
  {
    client_.setConnectionCallback(
        std::bind(&AddrBookClient::onConnection, this, _1));
    client_.setMessageCallback(
        std::bind(&ProtobufCodec::onMessage, &codec_, _1, _2, _3));
  }

  void connect()
  {
    client_.connect();
  }

  void onConnection(const TcpConnectionPtr& conn)
  {
    LOG_INFO << conn->localAddress().toHostPort() << " -> "
        << conn->peerAddress().toHostPort() << " is "
        << (conn->connected() ? "UP" : "DOWN");

    if (conn->connected())
    {
      codec_.send(conn, *messageToSend);
    }
    else
    {
      // 连接断开后自动退出
      loop_->quit();
    }
  }

  void onUnknownMessage(const TcpConnectionPtr&,
                        const MessagePtr& message,
                        Timestamp receiveTime)
  {
    LOG_INFO << "onUnknownMessage: " << message->GetTypeName();
  }
     
 private:
  EventLoop *loop_;
  TcpClient client_;
  ProtobufDispatcher dispatcher_;  
  ProtobufCodec codec_;

};


  
void PromptForAddress(tutorial::Person* person){
  cout<<"Enter person ID number: ";
  int id;
  cin>>id;
  person->set_id(id);
  cin.ignore(256,'\n'); // 忽略用户输入的换行符,因为下面要输入字符串

  cout<<"Enter name: ";
  getline(cin,*person->mutable_name());

  cout<<"Enter email address (blank for none): ";
  string email;
  getline(cin,email);
  if(!email.empty()){
    person->set_email(email);
  }

  while(true){
    cout<<"Enter a phone number(or leave blank to finsh): ";
    string number;
    getline(cin,number);
    if(number.empty()){
      break;
    }
    // 获取一个新的phone_number message来填充数值
    tutorial::Person::PhoneNumber* phone_number=person->add_phones();
    phone_number->set_number(number);
    
    cout<<"Is this a mobile, home, or work phone?";
    string type;
    getline(cin,type);
    if (type == "mobile") {
      phone_number->set_type(tutorial::Person::MOBILE);
    } else if (type == "home") {
      phone_number->set_type(tutorial::Person::HOME);
    } else if (type == "work") {
      phone_number->set_type(tutorial::Person::WORK);
    } else {
      cout << "Unknown phone type.  Using default." << endl;
    }
  }
}

  
int main(int argc, char* argv[])
{
  LOG_INFO << "pid = " << getpid();
  if (argc > 2)
  {
    EventLoop loop;
    uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
    InetAddress serverAddr(argv[1], port);

    tutorial::Person person;
    // 用户交互添加新person
    PromptForAddress(&person);
    messageToSend=&person;

    AddrBookClient client(&loop, serverAddr);
    client.connect();
    loop.loop();
  }
  else
  {
    printf("Usage: %s host_ip port [q|e]\n", argv[0]);
  }
}
