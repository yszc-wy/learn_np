/*
* Author:           yszc-wy@foxmail.com 
* Encoding:			    utf-8
* Description:      
*/

// 本类头文件

// C系统库头文件
#include <stdio.h>
// C++系统库头文件

// 第三方库头文件

// 本项目头文件
#include "base/AsyncLogging.h"
#include "base/LogFile.h"
#include "base/Timestamp.h"

using namespace yszc;

AsyncLogging::AsyncLogging(const string& basename,
                           off_t rollSize,
                           int flushInterval)
  : flushInterval_(flushInterval),
    running_(false),
    basename_(basename),
    rollSize_(rollSize),
    thread_(std::bind(&AsyncLogging::threadFunc, this), "Logging"),
    latch_(1),
    mutex_(),
    cond_(mutex_),
    currentBuffer_(new Buffer),
    nextBuffer_(new Buffer),
    buffers_()
{
  currentBuffer_->bzero();
  nextBuffer_->bzero();
  buffers_.reserve(16);
}


void AsyncLogging::append(const char* logline, int len)
{
  yszc::MutexLockGuard lock(mutex_);
  if(currentBuffer_->avail()>len)
  {
    currentBuffer_->append(logline,len);
  } 
  else {
    buffers_.push_back(std::move(currentBuffer_));
    if(nextBuffer_){
      currentBuffer_=std::move(nextBuffer_);
    } else{
      currentBuffer_.reset(new Buffer);  // Rarely happens
    }
    currentBuffer_->append(logline,len);
    cond_.notify();
  }
}


void AsyncLogging::threadFunc()
{
  assert(running_ == true);
  latch_.countDown();
  // 日志文件在线程中生成
  LogFile output(basename_, rollSize_, false);
  BufferPtr newBuffer1(new Buffer);
  BufferPtr newBuffer2(new Buffer);
  newBuffer1->bzero();
  newBuffer2->bzero();
  BufferVector buffersToWrite;
  buffersToWrite.reserve(16);
  while(running_)
  {
    assert(newBuffer1 && newBuffer1->length() == 0);
    assert(newBuffer2 && newBuffer2->length() == 0);
    assert(buffersToWrite.empty());
    {
      MutexLockGuard lock(mutex_);
      // 阻塞队列的一种形式
      if(buffers_.empty()){ // unusual usage! 因为只有单个线程作为接受者,这里要和waitForSeconds配合使用,如果定时时间已到就直接跳出if,如果是while就会在检测到buffers.empty后继续阻塞,所以不能使用while
        cond_.waitForSeconds(flushInterval_); 
      }
      // 将cur缓冲插入到buffers中
      buffers_.push_back(std::move(currentBuffer_));
      // 将cur和next重新填上空缓冲区
      currentBuffer_=std::move(newBuffer1);
      if(!nextBuffer_){
        nextBuffer_=std::move(newBuffer2);
      }
      // 交换
      buffersToWrite.swap(buffers_);
    }

    assert(!buffersToWrite.empty());
    
    // 缓冲区缓存数据过多
    // 超过限制后只保留2个日志,其他全部丢弃
    if (buffersToWrite.size() > 25)
    {
      char buf[256];
      snprintf(buf, sizeof buf, "Dropped log messages at %s, %zd larger buffers\n",
               Timestamp::now().toFormattedString().c_str(),
               buffersToWrite.size()-2);
      fputs(buf, stderr);
      output.append(buf, static_cast<int>(strlen(buf)));
      buffersToWrite.erase(buffersToWrite.begin()+2, buffersToWrite.end());
    }

    // 可以使用writeev
    for (const auto& buffer : buffersToWrite)
    {
      // FIXME: use unbuffered stdio FILE ? or use ::writev ?
      output.append(buffer->data(), buffer->length());
    }

    if (buffersToWrite.size() > 2)
    {
      // drop non-bzero-ed buffers, avoid trashing
      // 剩下的用来填充newbuffers
      buffersToWrite.resize(2);
    }

    if (!newBuffer1)
    {
      assert(!buffersToWrite.empty());
      newBuffer1 = std::move(buffersToWrite.back());
      buffersToWrite.pop_back();
      newBuffer1->reset();
    }

    if (!newBuffer2)
    {
      assert(!buffersToWrite.empty());
      newBuffer2 = std::move(buffersToWrite.back());
      buffersToWrite.pop_back();
      newBuffer2->reset();
    }

    buffersToWrite.clear();
    output.flush();
  }
  output.flush();
}

