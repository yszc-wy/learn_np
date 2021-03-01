/*
* Author:           
* Encoding:			    utf-8
* Description:      日志文件是对AppendFile的封装(组合的封装方式)
* 每隔一段时间或写入一定大小的数据就会换一个文件重新写入
* 将日志文件与文件系统的接口构造完毕
*/
#pragma once

// C系统库头文件

// C++系统库头文件
#include <memory>
// 第三方库头文件

// 本项目头文件
#include "base/Mutex.h"
#include "base/Types.h"



// 系统结构体或其他命名空间中类的前向声明(注意要包含相应的命名空间)


namespace yszc
{

namespace FileUtil
{
class AppendFile;
}

class LogFile : noncopyable
{
 public:
  LogFile(const string& basename,
          off_t rollSize,
          bool threadSafe = true,
          int flushInterval = 3,
          int checkEveryN = 1024);
  ~LogFile();

  void append(const char* logline, int len);
  void flush();
  bool rollFile();

 private:
  void append_unlocked(const char* logline, int len);

  // 生成日志名称
  static string getLogFileName(const string& basename, time_t* now);

  const string basename_;
  const off_t rollSize_; // ?
  const int flushInterval_;
  const int checkEveryN_;

  int count_; // 对每次写入进行计数,以此判断是否需要flush(checkEveryN_)
  

  std::unique_ptr<MutexLock> mutex_; // 我们要根据threadSafe选项来决定是否要分配mutex_
  time_t startOfPeriod_;
  time_t lastRoll_;
  time_t lastFlush_;
  // 使用unique_ptr是因为file随着时间和写入量在不断变化
  std::unique_ptr<FileUtil::AppendFile> file_;

  const static int kRollPerSeconds_ = 60*60*24;
};


} // namespace yszc

