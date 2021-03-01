/*
* Encoding:			    utf-8
* Description:      在第一次调用instance时创建,在线程结束时销毁
* (一定要销毁,因为线程销毁时只会消除指针类型,具体的数据是需要手动delete的)
* 线程内对象也不存在线程同步的问题
*/
#pragma once

// C系统库头文件

// C++系统库头文件
#include <assert.h>
#include <pthread.h>
// 第三方库头文件

// 本项目头文件
#include "base/noncopyable.h"
#include "base/Mutex.h"




// 系统结构体或其他命名空间中类的前向声明(注意要包含相应的命名空间)


namespace yszc
{

template <typename T>
class ThreadLocalSingleton : public noncopyable
{
 public: 
  ThreadLocalSingleton() =delete;
  ~ThreadLocalSingleton() = delete;

  static T& instance()
  {
    if(!t_value_)
    {
      t_value_=new T();
      deleter_.set(t_value_);
    }
    return *t_value_;
  }

  static T* pointer()
  {
    return t_value_;
  }
 private:
  static void destructor(void * obj)
  {
    assert(obj==t_value_);
    typedef char T_must_be_complete_type[sizeof(T)==0? -1:1];
    T_must_be_complete_type dummy; (void) dummy;
    // 释放该线程中分配的空间
    delete t_value_;
  }


  // 私有的deleter,不给其他人用,RAII管理
  // 虽然_thread类型就可以完成singleton的工作
  // 我还需要在线程结束时完成对T的析构,所以依然要使用
  // pthread_key_create来注册析构函数
  class Deleter
  {
   public:
    Deleter()
    {
      MCHECK(pthread_key_create(&pkey_,&ThreadLocalSingleton::destructor));
    }
    ~Deleter()
    {
      MCHECK(pthread_key_delete(pkey_));
    }
    void set(T* obj)
    {
      assert(pthread_getspecific(pkey_)==NULL);
      MCHECK(pthread_setspecific(pkey_,obj));
    }
    pthread_key_t pkey_;
  };
  static __thread T* t_value_;
  static Deleter deleter_;
};

// 静态声明
template<typename T>
__thread T* ThreadLocalSingleton<T>::t_value_ = 0;

template<typename T>
typename ThreadLocalSingleton<T>::Deleter ThreadLocalSingleton<T>::deleter_;

} // namespace yszc

