/*
* Encoding:			    utf-8
* Description:      
*/

// C系统库头文件

// C++系统库头文件

// 第三方库头文件
// 本项目头文件
#include "base/noncopyable.h"
#include "base/Timestamp.h"
#include "base/Logging.h"
#include "net/EventLoop.h"
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
  AddrBookServer(EventLoop* loop,
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






using namespace yszc;
using namespace std::placeholders;

void ListPhoneNumber(const tutorial::Person_PhoneNumber& phone_number)
{
  switch (phone_number.type()) {
    case tutorial::Person::MOBILE:
      LOG_INFO << "  Mobile phone #: ";
      break;
    case tutorial::Person::HOME:
      LOG_INFO << "  Home phone #: ";
      break;
    case tutorial::Person::WORK:
      LOG_INFO << "  Work phone #: ";
      break;
  }
  LOG_INFO << phone_number.number() ;
}
void ListPerson(const tutorial::Person& person){
  LOG_INFO << "Person ID: " << person.id();
  LOG_INFO << "  Name: " << person.name();
  if (person.has_email()) {
    LOG_INFO << "  E-mail address: " << person.email() ;
  }

  for (int j = 0; j < person.phones_size(); j++) {
    const tutorial::Person::PhoneNumber& phone_number = person.phones(j);
    ListPhoneNumber(phone_number);
  }
}
void ListAddressBook(const tutorial::AddressBook& address_book){
  for(int i=0;i<address_book.people_size();++i){
    const tutorial::Person& person=address_book.people(i);
    ListPerson(person);
  }
}

AddrBookServer::AddrBookServer(EventLoop* loop,
               const yszc::net::InetAddress& listenAddr)
  :server_(loop,listenAddr,"Addrbook Server"),
   dispatcher_(std::bind(&AddrBookServer::onUnknownMessage,this,_1,_2,_3)),
   codec_(std::bind(&yszc::net::ProtobufDispatcher::onProtobufMessage,&dispatcher_,_1,_2,_3))
{
  // 使用<>来指明想要实例化的版本
  dispatcher_.registerMessageCallback<tutorial::AddressBook>(
          std::bind(&AddrBookServer::onAddressBook,this,_1,_2,_3));
  dispatcher_.registerMessageCallback<tutorial::Person>(
          std::bind(&AddrBookServer::onPerson,this,_1,_2,_3));
  dispatcher_.registerMessageCallback<tutorial::Person_PhoneNumber>(
          std::bind(&AddrBookServer::onPhoneNumber,this,_1,_2,_3));

  server_.setMessageCallback(
          std::bind(&yszc::net::ProtobufCodec::onMessage, &codec_ ,_1,_2,_3));
}

void AddrBookServer::onConnection(const yszc::net::TcpConnectionPtr& conn)
{
  
  LOG_INFO << "AddrBookServer - " << conn->peerAddress().toHostPort() << " -> "
           << conn->localAddress().toHostPort() << " is "
           << (conn->connected() ? "UP" : "DOWN");
}



void AddrBookServer::onUnknownMessage(const yszc::net::TcpConnectionPtr& conn,
                      const MessagePtr& message,
                      Timestamp receiveTime)
{
  LOG_INFO << "onUnknownMessage: " << message->GetTypeName();
  conn->shutdown();
}

void AddrBookServer::onPerson(const yszc::net::TcpConnectionPtr& conn,
              const PersonPtr& person,
              Timestamp receiveTime)
{
  ListPerson(*person);
  conn->shutdown();
}

void AddrBookServer::onPhoneNumber(const yszc::net::TcpConnectionPtr& conn,
              const PhoneNumberPtr& phone_number,
              Timestamp receiveTime)
{
  ListPhoneNumber(*phone_number);
  conn->shutdown();
}

void AddrBookServer::onAddressBook(const yszc::net::TcpConnectionPtr& conn,
              const AddrBookPtr& address_book,
              Timestamp receiveTime)
{
  ListAddressBook(*address_book);
  conn->shutdown();
}


int main(int argc, char* argv[]){
  LOG_INFO << "pid = " << getpid();
  if (argc > 1)
  {
    EventLoop loop;
    uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
    yszc::net::InetAddress serverAddr(port);
    AddrBookServer server(&loop, serverAddr);
    server.start();
    loop.loop();
  }
  else
  {
    printf("Usage: %s port\n", argv[0]);
  }
}

