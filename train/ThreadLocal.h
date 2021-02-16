/*
* Author:           yszc-wy@foxmail.com 
* Encoding:			    utf-8
* Description:      
*/
#pragma once

// C系统库头文件
#include <pthread.h>
// C++系统库头文件

// 第三方库头文件

// 本项目头文件


namespace train
{

template <typename T>
class ThreadLocal
{
 public:
  ThreadLocal()
  {
    pthread_key_create(&key_,&ThreadLocal::destructor);
  }
  ~ThreadLocal()
  {
    pthread_key_delete(key_);
  }
  T& get(){
    T* obj= static_cast<T*>(pthread_getspecific(key_));
    if(!obj){
      obj=new T();
      pthread_setspecific(key_,obj);
    }
    return &obj;
  }
  static void destructor(void *obj){
    delete static_cast<T*>(obj);
  }
 private:
  pthread_key_t key_;
};


}

