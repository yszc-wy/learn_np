/*
* Author:           
* Encoding:			    utf-8
* Description:      RAII文件管理方式
*/
#pragma once

// C系统库头文件
#include <sys/types.h>  // for off_t
// C++系统库头文件

// 第三方库头文件

// 本项目头文件
#include "base/noncopyable.h"
#include "base/StringPiece.h"



// 系统结构体或其他命名空间中类的前向声明(注意要包含相应的命名空间)


namespace yszc
{

namespace FileUtil
{

// read small file < 64KB
class ReadSmallFile : noncopyable
{
 public:
  ReadSmallFile(StringArg filename);
  ~ReadSmallFile();

  // return errno
  // 为何要使用模版
  // 文件读取与错误处理
  template<typename String>
  int readToString(int maxSize,
                   String* content,
             // 获取文件的相关信息
                   int64_t* fileSize,
                   int64_t* modifyTime,
                   int64_t* createTime);

  /// Read at maxium kBufferSize into buf_
  // return errno
  // 尾部会加上`\0`
  int readToBuffer(int* size);

  const char* buffer() const { return buf_; }

  static const int kBufferSize = 64*1024;

 private:
  int fd_;
  int err_; // 记录errno
  char buf_[kBufferSize];
};


// read the file content, returns errno if error happens.
template<typename String>
int readFile(StringArg filename,
             int maxSize,
             String* content,
             int64_t* fileSize = NULL,
             int64_t* modifyTime = NULL,
             int64_t* createTime = NULL)
{
  ReadSmallFile file(filename);
  return file.readToString(maxSize, content, fileSize, modifyTime, createTime);
}


// not thread safe
class AppendFile : noncopyable
{
 public:
  explicit AppendFile(StringArg filename);

  ~AppendFile();

  void append(const char* logline, size_t len);

  void flush();

  off_t writtenBytes() const { return writtenBytes_; }

 private:

  size_t write(const char* logline, size_t len);

  FILE* fp_;
  // 自定义写缓冲大小,越大写越块
  char buffer_[64*1024];
  off_t writtenBytes_;
};

} // namespace FileUtil
}  // namespace yszc

