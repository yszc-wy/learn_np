/*
* Encoding:			    utf-8
* Description:      
*/
#pragma once

// C系统库头文件
#include <pthread.h>
// C++系统库头文件

// 第三方库头文件

// 本项目头文件


namespace yszc
{

template <typename T>
class ThreadLocalSingleton
{
 public:  
  ThreadLocalSingleton()=delete ;
  ~ThreadLocalSingleton()=delete ;
  static T& instance(){
    if(!val_){
      val_=new T();
      deleter_.set(val_);
    }
    return *val_;
  }
 private:

  static void destructor(void *obj){
    delete static_cast<T*>(obj);
  }
  class Deleter
  {
   public:
    Deleter(){
      pthread_key_create(&key_,&ThreadLocalSingleton::destructor);
    }
    void set(T* obj){
      pthread_setspecific(key_,obj);
    }
    pthread_key_t key_;
  };
  static __thread T* val_;
  // 这里是真正的全局变量,全部线程公用一个key就行
  static Deleter deleter_;
};

template <typename T>
__thread T* ThreadLocalSingleton<T>::val_=NULL;

template<typename T>
typename ThreadLocalSingleton<T>::Deleter ThreadLocalSingleton<T>::deleter_;

}

