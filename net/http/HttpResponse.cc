/*
* Encoding:			    utf-8
* Description:      
*/

// 本类头文件
#include "net/http/HttpResponse.h"
// C系统库头文件
#include <stdio.h>
// C++系统库头文件

// 第三方库头文件

// 本项目头文件
#include "net/Buffer.h"

using namespace yszc;
using namespace yszc::net;

void HttpResponse::appendToBuffer(Buffer* output) const
{
  char buf[32];
  snprintf(buf, sizeof buf, "HTTP/1.1 %d ", statusCode_);
  output->append(buf);
  output->append(statusMessage_);
  output->append("\r\n");

  if (closeConnection_)
  {
    output->append("Connection: close\r\n");
  }
  else
  {
    snprintf(buf, sizeof buf, "Content-Length: %zd\r\n", body_.size());
    output->append(buf);
    output->append("Connection: Keep-Alive\r\n");
  }

  for (const auto& header : headers_)
  {
    output->append(header.first);
    output->append(": ");
    output->append(header.second);
    output->append("\r\n");
  }

  output->append("\r\n");
  output->append(body_);
}
