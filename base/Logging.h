/*
* Author:           yszc-wy@foxmail.com 
* Encoding:			    utf-8
* Description:      日志系统
* Logging.h包含了字符串的预写和loging的配置和整体的log框架
* Logging时临时变量,创建时会写入日期等基本配置,析构会提取buffer写入output函数
* LoggingStream负责构造字符串流的符号
* Logging使用Stream来构造字符串,然后将构造好的字符串使用g_output来处理
* g_output可以注册普通输出函数,或者是异步日志
* Logging包含了对全局变量的注册函数,全局变量并不在.h文件中,只提供了接口访问,是个好方法
*/
#pragma once

// C系统库头文件

// C++系统库头文件

// 第三方库头文件

// 本项目头文件
#include "base/LogStream.h"
#include "base/Timestamp.h"



// 系统结构体或其他命名空间中类的前向声明(注意要包含相应的命名空间)


namespace yszc
{

class TimeZone;

class Logger
{
 public:
  enum LogLevel
  {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
    NUM_LOG_LEVELS,
  };

  // compile time calculation of basename of source file
  // 解析文件名路径并保存
  class SourceFile
  {
   public:
    template<int N>
    SourceFile(const char (&arr)[N])
      : data_(arr),
        size_(N-1)
    {
      const char* slash = strrchr(data_, '/'); // builtin function
      if (slash)
      {
        data_ = slash + 1;
        size_ -= static_cast<int>(data_ - arr);
      }
    }

    explicit SourceFile(const char* filename)
      : data_(filename)
    {
      const char* slash = strrchr(filename, '/');
      if (slash)
      {
        data_ = slash + 1;
      }
      size_ = static_cast<int>(strlen(data_));
    }

    const char* data_;
    int size_;
  };

  Logger(SourceFile file, int line);
  Logger(SourceFile file, int line, LogLevel level);
  Logger(SourceFile file, int line, LogLevel level, const char* func);
  Logger(SourceFile file, int line, bool toAbort);
  ~Logger();

  LogStream& stream() { return impl_.stream_; }

  static LogLevel logLevel();
  static void setLogLevel(LogLevel level);

  typedef void (*OutputFunc)(const char* msg, int len);
  typedef void (*FlushFunc)();
  static void setOutput(OutputFunc);
  static void setFlush(FlushFunc);
  static void setTimeZone(const TimeZone& tz);

 private:

class Impl
{
 public:
  typedef Logger::LogLevel LogLevel;
  Impl(LogLevel level, int old_errno, const SourceFile& file, int line);
  void formatTime();
  void finish();

  Timestamp time_;
  LogStream stream_;
  LogLevel level_;
  int line_;
  SourceFile basename_;
};

  Impl impl_;
};


extern Logger::LogLevel g_logLevel;

inline Logger::LogLevel Logger::logLevel()
{
  return g_logLevel;
}

//
// 不要这样写!!!
//
// if (good)
//   LOG_INFO << "Good news";
// else
//   LOG_WARN << "Bad news";
//
// this expends to
//
// if (good)
//   if (logging_INFO)
//     logInfoStream << "Good news";
//   else
//     logWarnStream << "Bad news";
//
// 文件,行号,错误等级,是否abort
#define LOG_TRACE if (yszc::Logger::logLevel() <= yszc::Logger::TRACE) \
  yszc::Logger(__FILE__, __LINE__, yszc::Logger::TRACE, __func__).stream()
#define LOG_DEBUG if (yszc::Logger::logLevel() <= yszc::Logger::DEBUG) \
  yszc::Logger(__FILE__, __LINE__, yszc::Logger::DEBUG, __func__).stream()
#define LOG_INFO if (yszc::Logger::logLevel() <= yszc::Logger::INFO) \
  yszc::Logger(__FILE__, __LINE__).stream()
#define LOG_WARN yszc::Logger(__FILE__, __LINE__, yszc::Logger::WARN).stream()
#define LOG_ERROR yszc::Logger(__FILE__, __LINE__, yszc::Logger::ERROR).stream()
#define LOG_FATAL yszc::Logger(__FILE__, __LINE__, yszc::Logger::FATAL).stream()
#define LOG_SYSERR yszc::Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL yszc::Logger(__FILE__, __LINE__, true).stream()


const char* strerror_tl(int savedErrno);

// 简单的辅助宏
// Taken from glog/logging.h
//
// Check that the input is non NULL.  This very useful in constructor
// initializer lists.

#define CHECK_NOTNULL(val) \
  ::yszc::CheckNotNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))

// 检查是否为空,为空就写入Logger并报错
// A small helper for CHECK_NOTNULL().
template <typename T>
T* CheckNotNull(Logger::SourceFile file, int line, const char *names, T* ptr)
{
  if (ptr == NULL)
  {
   Logger(file, line, Logger::FATAL).stream() << names;
  }
  return ptr;
}

}

