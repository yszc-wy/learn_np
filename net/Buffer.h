/*
* Author:           
* Encoding:			    utf-8
* Description:      
*/
#pragma once

// C系统库头文件

// C++系统库头文件
#include <assert.h>
#include <vector>
#include <algorithm>
#include <string>
// 第三方库头文件
#include "base/copyable.h"
// 本项目头文件
#include "SocketsOps.h"




// 系统结构体或其他命名空间中类的前向声明(注意要包含相应的命名空间)


namespace yszc
{

namespace net
{

// 本命名空间内部类的前向声明
class Buffer: public copyable
{
 public:
  static const size_t kCheapPrepend = 8; // 留1b前置空间
  static const size_t kInitialSize = 1024; // 初始化1kb

  Buffer()
    : buffer_(kCheapPrepend + kInitialSize),
      readerIndex_(kCheapPrepend),
      writerIndex_(kCheapPrepend)
  {
    assert(readableBytes() == 0);
    assert(writableBytes() == kInitialSize);
    assert(prependableBytes() == kCheapPrepend);
  }


  void swap(Buffer& rhs)
  {
    buffer_.swap(rhs.buffer_);
    std::swap(readerIndex_, rhs.readerIndex_);
    std::swap(writerIndex_, rhs.writerIndex_);
  }


  // 获取读指针
  const char* peek() const
  { return begin() + readerIndex_; }

  int32_t peekInt32() const
  { 
    assert(readableBytes()>=sizeof(int32_t));
    return sockets::networkToHost32
      (*reinterpret_cast<const int32_t*>(peek()));
  }
  int64_t peekInt64() const
  { 
    assert(readableBytes()>=sizeof(int64_t));
    return sockets::networkToHost64
      (*reinterpret_cast<const int64_t*>(peek()));
  }

  const char* findCRLF() const
  {
    // FIXME: replace with memmem()?
    const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF+2);
    return crlf == beginWrite() ? NULL : crlf;
  }

  const char* findCRLF(const char* start) const
  {
    assert(peek() <= start);
    assert(start <= beginWrite());
    // FIXME: replace with memmem()?
    const char* crlf = std::search(start, beginWrite(), kCRLF, kCRLF+2);
    return crlf == beginWrite() ? NULL : crlf;
  }

  size_t readableBytes() const
  { return writerIndex_ - readerIndex_; }

  size_t writableBytes() const
  { return buffer_.size() - writerIndex_; }

  size_t prependableBytes() const
  { return readerIndex_; }


  // 下面的函数用于略过部分数据
  // 返回值为void,防止出现下面的操作
  // string str(retrieve(readableBytes()), readableBytes());
  // 上面的操作用于提取所有可读的部分并放到str中,但这里的2个函数调用顺序不一定
  // 导致readableBytes的返回值不是一定的,可能造成readableBytes返回0
  void retrieve(size_t len)
  {
    assert(len <= readableBytes());
    readerIndex_ += len;
  }
  
  // 专门提炼一个接口防止上面的问题
  std::string retrieveAsString()
  {
    std::string str(peek(), readableBytes());
    retrieveAll();
    return str;
  }


  void ensureWritableBytes(size_t len)
  {
    if (writableBytes() < len)
    {
      makeSpace(len); // 分配空间
    }
    assert(writableBytes() >= len);
  }

  void retrieveAll()
  {
    readerIndex_ = kCheapPrepend;
    writerIndex_ = kCheapPrepend;
  }

  void retrieveUntil(const char* end)
  {
    assert(peek() <= end);
    assert(end <= beginWrite());
    retrieve(end - peek());
  }


  // append函数
  void append(const std::string& str)
  {
    append(str.data(), str.length());
  }

  void append(const char*  data, size_t len)
  {
    ensureWritableBytes(len); // 保证可插入长度
    std::copy(data, data+len, beginWrite()); // 拷贝到指定的位置
    hasWritten(len); // 重新定位writeindex
  }

  void append(const void* /*restrict*/ data, size_t len)
  {
    append(static_cast<const char*>(data), len);
  }

  ///
  /// Append int64_t using network endian
  ///
  void appendInt64(int64_t x)
  {
    int64_t be64 = sockets::hostToNetwork64(x);
    append(&be64, sizeof be64);
  }

  ///
  /// Append int32_t using network endian
  ///
  void appendInt32(int32_t x)
  {
    // 自动转换成网络字节序
    int32_t be32 = sockets::hostToNetwork32(x);
    append(&be32, sizeof be32);
  }

  void appendInt16(int16_t x)
  {
    int16_t be16 = sockets::hostToNetwork16(x);
    append(&be16, sizeof be16);
  }

  void appendInt8(int8_t x)
  {
    append(&x, sizeof x);
  }

  // 这里判断大小,但不进行动态扩展
  void prepend(const void* /*restrict*/ data, size_t len)
  {
    assert(len <= prependableBytes());
    readerIndex_ -= len;
    const char* d = static_cast<const char*>(data);
    std::copy(d, d+len, begin()+readerIndex_);
  }

  char* beginWrite()
  { return begin() + writerIndex_; }

  const char* beginWrite() const
  { return begin() + writerIndex_; }

  void hasWritten(size_t len)
  { writerIndex_ += len; }


  // 缩小buf体积
  void shrink(size_t reserve)
  {
   std::vector<char> buf(kCheapPrepend+readableBytes()+reserve);
   std::copy(peek(), peek()+readableBytes(), buf.begin()+kCheapPrepend);
   buf.swap(buffer_);
  }

  /// Read data directly into buffer.
  ///
  /// It may implement with readv(2)
  /// @return result of read(2), @c errno is saved
  ssize_t readFd(int fd, int* savedErrno);
 private:

  char* begin()
  { return &*buffer_.begin(); }

  const char* begin() const
  { return &*buffer_.begin(); }

  void makeSpace(size_t len)
  {
    // 看看剩余空间是否足够
    if (writableBytes() + prependableBytes() < len + kCheapPrepend)
    {
      // 不够就继续分配
      buffer_.resize(writerIndex_+len);
    }
    else
    {
      // 足够就通过移动获取多余的后缀空间
      // move readable data to the front, make space inside buffer
      assert(kCheapPrepend < readerIndex_);
      size_t readable = readableBytes();
      std::copy(begin()+readerIndex_,
                begin()+writerIndex_,
                begin()+kCheapPrepend);
      readerIndex_ = kCheapPrepend;
      writerIndex_ = readerIndex_ + readable;
      assert(readable == readableBytes());
    }
  }

 private:
  std::vector<char> buffer_;
  size_t readerIndex_;
  size_t writerIndex_;

  static const char kCRLF[];
};
}

}

