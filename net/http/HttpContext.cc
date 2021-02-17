
/*
* Author:           yszc-wy@foxmail.com 
* Encoding:			    utf-8
* Description:      
*/

// 本类头文件

// C系统库头文件

// C++系统库头文件

// 第三方库头文件

// 本项目头文件
#include "net/Buffer.h"
#include "net/http/HttpContext.h"

using namespace yszc;
using namespace yszc::net;

// 处理请求头
bool HttpContext::processRequestLine(const char* begin, const char* end)
{
  // 使用start和space区间包围要解码的区域然后传递给request
  bool succeed = false;
  const char* start = begin;
  const char* space = std::find(start, end, ' ');
  if (space != end && request_.setMethod(start, space))
  {
    start = space+1;
    space = std::find(start, end, ' ');
    if (space != end)
    {
      const char* question = std::find(start, space, '?');
      // 如果有question(/index?[question])
      if (question != space)
      {
        request_.setPath(start, question);
        request_.setQuery(question, space);
      }
      else
      {
        request_.setPath(start, space);
      }
      start = space+1;
      // 判断解析成果的依旧是剩余大小为8,且前7个等于HTTP/1.
      succeed = end-start == 8 && std::equal(start, end-1, "HTTP/1.");
      
      if (succeed)
      {
        // 判断版本号码
        if (*(end-1) == '1')
        {
          request_.setVersion(HttpRequest::kHttp11);
        }
        else if (*(end-1) == '0')
        {
          request_.setVersion(HttpRequest::kHttp10);
        }
        else
        {
          succeed = false;
        }
      }
    }
  }
  return succeed;
}

// return false if any error
// 根据httpcontext的状态决定如何对下一行进行解析
bool HttpContext::parseRequest(Buffer* buf, Timestamp receiveTime)
{
  bool ok = true;
  bool hasMore = true;
  while (hasMore)
  {
    if (state_ == kExpectRequestLine)
    {
      const char* crlf = buf->findCRLF();
      if (crlf)
      {
        // 解析请求行
        ok = processRequestLine(buf->peek(), crlf);
        if (ok)
        {
          request_.setReceiveTime(receiveTime);
          buf->retrieveUntil(crlf + 2);
          state_ = kExpectHeaders;
        }
        else
        {
          hasMore = false;
        }
      }
      // 不构成完整的一行,结束
      else
      {
        hasMore = false;
      }
    }
    else if (state_ == kExpectHeaders)
    {
      const char* crlf = buf->findCRLF();
      if (crlf)
      {
        // 解析头部
        const char* colon = std::find(buf->peek(), crlf, ':');
        if (colon != crlf)
        {
          // 添加头部
          request_.addHeader(buf->peek(), colon, crlf);
          // 如果后面还有行会继续循环
        }
        else
        {
          // 如果后面没有行,就结束循环,并设置成功解析状态
          // empty line, end of header
          // FIXME:
          state_ = kGotAll;
          hasMore = false;
        }
        buf->retrieveUntil(crlf + 2);
      }
      else
      {
        hasMore = false;
      }
    }
    else if (state_ == kExpectBody)
    {
      // FIXME:
    }
  }
  return ok;
}
