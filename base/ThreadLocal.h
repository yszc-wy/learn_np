/*
* Author:           yszc-wy@foxmail.com 
* Encoding:			    utf-8
* Description:      使用线程特定数据函数调用创建每个线程特定的类对象
* pthread_key_create在内核中分配一个key,每个key在线程中映射到一个特定的void指针,我们可以将分配好的数据地址赋值给void指针
*/
#pragma once

// C系统库头文件

// C++系统库头文件

// 第三方库头文件

// 本项目头文件
#include "noncopyable.h"
#include "Mutex.h"



// 系统结构体或其他命名空间中类的前向声明(注意要包含相应的命名空间)


namespace yszc
{

template <typename T>
class ThreadLocal: public noncopyable
{
 public:
  ThreadLocal()
  {
    MCHECK(pthread_key_create(&pkey_,&ThreadLocal::destructor));
  }

  ~ThreadLocal()
  {
    MCHECK(pthread_key_delete(pkey_));
  }

  // 只有在调用时才会分配并初始化该类
  T& value()
  {
    T* perThreadValue = static_cast<T*>(pthread_getspecific(pkey_));
    if (!perThreadValue)
    {
      T* newObj = new T();
      // 为void指针赋值
      MCHECK(pthread_setspecific(pkey_, newObj));
      perThreadValue = newObj;
    }
    return *perThreadValue;
  }
 private:
  
  // 负责在线程结束时析构
  static void destructor(void *x)
  {
    T* obj = static_cast<T*>(x);
    // T必须是一个完整类型,下面这种检测类型的写法保证了编译期间就可以报错(利用非法下标)
    typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
    T_must_be_complete_type dummy; (void) dummy;
    delete obj;
  }
  
  pthread_key_t pkey_;
};

} // namespace yszc

