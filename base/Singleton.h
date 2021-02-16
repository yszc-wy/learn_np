/*
* Author:           yszc-wy@foxmail.com 
* Encoding:			    utf-8
* Description:      单例模式
*/
#pragma once

// C系统库头文件
#include <assert.h>
#include <pthread.h>
#include <stdlib.h> // atexit
// C++系统库头文件
// 第三方库头文件
// 本项目头文件
#include "base/noncopyable.h"




// 系统结构体或其他命名空间中类的前向声明(注意要包含相应的命名空间)


namespace yszc
{

namespace detail
{
// This doesn't detect inherited member functions!
// http://stackoverflow.com/questions/1966362/sfinae-to-check-for-inherited-member-functions
// 该模版用于检测类型T中是否包含指定的
template<typename T>
struct has_no_destroy
{
  // 定义返回值为char和int32_t类型的静态函数
  // 对于传入的类型C,若C存在no_destroy成员函数
  // decltype接受C::no_destroy函数的地址,然后解析出函数指针类型
  // test<T>(0)会自动匹配返回值为char的test函数
  // 然后sizeof(test<T>(0))自然返回1
  template <typename C> static char test(decltype(&C::no_destroy));
  template <typename C> static int32_t test(...);
  const static bool value = sizeof(test<T>(0)) == 1;
};

}

template <typename T>
class Singleton : noncopyable
{
 public:
  Singleton() = delete;
  ~Singleton() = delete;

  static T& instance()
  {
    pthread_once(&ponce_,&Singleton::init);
    assert(value_!=NULL);
    return *value_;
  }

 private:
  static void init()
  {
    value_ = new T();
    // 注册析构函数
    if( !detail::has_no_destroy<T>::value)
    {
      ::atexit(destroy);
    }
  }

  static void destroy()
  {
    typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
    T_must_be_complete_type dummy; (void) dummy;

    delete value_;
    value_ = NULL;
  }
 private:

  static pthread_once_t ponce_;
  static T* value_;
};
// 全局变量初始化
template<typename T>
pthread_once_t Singleton<T>::ponce_=PTHREAD_ONCE_INIT;

template<typename T>
T* Singleton<T>::value_ = NULL;

}

