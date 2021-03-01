/*
* Author:           
* Encoding:			    utf-8
* Description:      
*/

// 本类头文件
#include "net/TimeWheel.h"
// C系统库头文件

// C++系统库头文件

// 第三方库头文件

// 本项目头文件
#include "net/TcpConnection.h"

using namespace yszc;
using namespace yszc::net;


TimeWheel::Entry::~Entry()
{
  TcpConnectionPtr ptr=weakptr_.lock();
  if(ptr){
    ptr->shutdown();
  }
}
