/*
* Author:           yszc-wy@foxmail.com 
* Encoding:			    utf-8
* Description:      
*/

// 本类头文件

// C系统库头文件
#include <assert.h>
#include <stdio.h>
#include <time.h>
// C++系统库头文件

// 第三方库头文件

// 本项目头文件
#include "base/LogFile.h"
#include "base/FileUtil.h"
#include "base/ProcessInfo.h"

using namespace yszc;

LogFile::LogFile(const string& basename,
                 off_t rollSize,
                 bool threadSafe,
                 int flushInterval,
                 int checkEveryN)
  : basename_(basename),
    rollSize_(rollSize),
    flushInterval_(flushInterval),
    checkEveryN_(checkEveryN),
    count_(0),
    mutex_(threadSafe ? new MutexLock : NULL),
    startOfPeriod_(0),
    lastRoll_(0),
    lastFlush_(0)
{
  // 要保证basename是个基本名
  assert(basename.find('/') == string::npos);
  // 第一次要使用rollFile初始化文件
  rollFile();
}

LogFile::~LogFile() = default;

// 如果只有一个线程写入该文件,我们可以使用无锁版本,效率高
// 否则就加锁
void LogFile::append(const char* logline, int len)
{
  if (mutex_)
  {
    MutexLockGuard lock(*mutex_);
    append_unlocked(logline, len);
  }
  else
  {
    append_unlocked(logline, len);
  }
}


void LogFile::flush()
{
  if (mutex_)
  {
    MutexLockGuard lock(*mutex_);
    file_->flush();
  }
  else
  {
    file_->flush();
  }
}


void LogFile::append_unlocked(const char* logline, int len)
{
  file_->append(logline, len);

  // 文件达到指定大小就重新写入新文件
  if (file_->writtenBytes() > rollSize_)
  {
    rollFile();
  }
  else
  {
    ++count_;
    // 每过N次就进行一次rollback的检查和刷新操作
    if (count_ >= checkEveryN_)
    {
      count_ = 0;
      time_t now = ::time(NULL);
      time_t thisPeriod_ = now / kRollPerSeconds_ * kRollPerSeconds_;
      // time进入了下一个时间阶段,就rollFile
      if (thisPeriod_ != startOfPeriod_)
      {
        rollFile();
      }
      // 否则根据FlushInterval进行一次flush
      else if (now - lastFlush_ > flushInterval_)
      {
        lastFlush_ = now;
        file_->flush();
      }
    }
  }
}


bool LogFile::rollFile()
{
  time_t now = 0;
  // 获取文件创建时间
  string filename = getLogFileName(basename_, &now);
  time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;

  if (now > lastRoll_)
  {
    // 记录上一个日志文件的创建时间
    lastRoll_ = now;
    lastFlush_ = now;
    // 记录上一个日志文件所在的时间阶段
    startOfPeriod_ = start;
    file_.reset(new FileUtil::AppendFile(filename));
    return true;
  }
  return false;
}


// 文件名,创建日期,主机名,进程号,后缀
string LogFile::getLogFileName(const string& basename, time_t* now)
{
  string filename;
  filename.reserve(basename.size() + 64);
  filename = basename;

  char timebuf[32];
  struct tm tm;
  *now = time(NULL);
  gmtime_r(now, &tm); // FIXME: localtime_r ?
  strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S.", &tm);
  filename += timebuf;

  filename += ProcessInfo::hostname();

  char pidbuf[32];
  snprintf(pidbuf, sizeof pidbuf, ".%d", ProcessInfo::pid());
  filename += pidbuf;

  filename += ".log";

  return filename;
}
