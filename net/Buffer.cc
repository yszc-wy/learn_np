/*
* Author:           
* Encoding:			    utf-8
* Description:      
*/

// 本类头文件
#include "Buffer.h"
// C系统库头文件

// C++系统库头文件
#include <errno.h>
#include <memory.h>
#include <sys/uio.h>
// 第三方库头文件
#include "base/Logging.h"
// 本项目头文件
#include "SocketsOps.h"

using namespace yszc;
using namespace yszc::net;


const char Buffer::kCRLF[] = "\r\n";


// 用户缓冲->
ssize_t Buffer::readFd(int fd, int* savedErrno)
{
  char extrabuf[65536];
  struct iovec vec[2];
  const size_t writable = writableBytes();
  vec[0].iov_base = begin()+writerIndex_;
  vec[0].iov_len = writable;
  // 分散缓冲区
  vec[1].iov_base = extrabuf;
  vec[1].iov_len = sizeof extrabuf;
  // 使用分散写策略,第一部分写到buff中,第二部分写到extrabuf中
  // 不重复读取,一是系统调用的次数考虑,二是由于采用电平触发下一次也会调用,三是公平性考虑
  const ssize_t n = readv(fd, vec, 2);
  if (n < 0) {
    *savedErrno = errno;
  } else if (implicit_cast<size_t>(n) <= writable) {
    writerIndex_ += n;
  } else {
    writerIndex_ = buffer_.size();
    append(extrabuf, n - writable);
  }
  return n;
}
