/*
* Author:           yszc-wy@foxmail.com 
* Encoding:			    utf-8
* Description:      四缓冲日志系统
* 多个前端通过append调用向currentBuffer_写入logline,如果currentBuffer_已满就将其插入到fullBuffers中
* 然后使用空闲缓冲区nextBuffer_或新分配的缓冲区继续进行插入并通知后端处理fullbuffer
* 后端定时/响应通知将currentBuffer中的内容插入到fullBuffer中,将writebuffer中之前留下的空闲空间重新为currentbuffer和nextbuffer赋值
* 然后将fullbuffer中的内容与writebuffer交换并在临界区外执行磁盘写入操作,来保证writebuffer的写入磁盘操作和currentbuffer的写入操作可以同步进行
* 和leveldb中的双缓冲有点类似,但是leveldb在写满memtable后会如果immtable不为空(说明磁盘io还没有结束),就会阻塞,等待写入完成
* 但是日志库不应该阻塞原进程,所以使用了上述的无阻塞设计,前端可以无阻塞的将数据写入后端,同时为了防止缓存数据时间过长导致丢失的问题,每3秒进行一次写入操作
* currentbuffer接受前端写入 (cache A)
* nextBuffer从后端接受空闲空间,以减少内存的分配 (cache B)
* 后端 newbuffer1 (cache C)
* 后端 newbuffer2 (cache D)
* 在bufferwrite的前2个缓冲区一直都是A,B,C,D这原始的4个缓冲区的一个,额外分配的缓冲区都会销毁 
* 异步日志的本质是前端不能因为后端的处理速度问题而阻塞,所以其存在的问题是生产速度高于消费速度时会造成内存的损耗和性能问题
* 为了避免日志系统对原程序产生影响,我们在buffers过大时直接丢弃
*/
#pragma once

// C系统库头文件

// C++系统库头文件
#include <atomic>
#include <vector>
// 第三方库头文件

// 本项目头文件
#include "base/BlockingQueue.h"
#include "base/BoundedBlockingQueue.h"
#include "base/CountDownLatch.h"
#include "base/Mutex.h"
#include "base/Thread.h"
#include "base/LogStream.h"




// 系统结构体或其他命名空间中类的前向声明(注意要包含相应的命名空间)


namespace yszc
{

class AsyncLogging : noncopyable
{
 public:
  AsyncLogging(const string& basename,
               off_t rollSize,
               int flushInterval = 3);

  ~AsyncLogging()
  {
    if (running_)
    {
      stop();
    }
  }

  void append(const char* logline, int len);

  void start()
  {
    running_ = true;
    thread_.start();
    latch_.wait();
  }

  // 使用condition通知线程开始运行
  // 将running=false,这样thread就会结束
  void stop() NO_THREAD_SAFETY_ANALYSIS
  {
    running_ = false;
    cond_.notify();
    thread_.join();
  }
 private:
  void threadFunc();

  // 没有拷贝代价,可以自动管理生命周期
  typedef yszc::detail::FixedBuffer<yszc::detail::kLargeBuffer> Buffer;
  typedef std::vector<std::unique_ptr<Buffer>> BufferVector;
  typedef BufferVector::value_type BufferPtr;

  // 刷新的时间间隔
  const int flushInterval_;
  std::atomic<bool> running_;
  const string basename_;
  const off_t rollSize_;
  yszc::Thread thread_;
  yszc::CountDownLatch latch_;
  yszc::MutexLock mutex_;
  yszc::Condition cond_ GUARDED_BY(mutex_);
  BufferPtr currentBuffer_ GUARDED_BY(mutex_);
  BufferPtr nextBuffer_ GUARDED_BY(mutex_);
  BufferVector buffers_ GUARDED_BY(mutex_);
};


}

