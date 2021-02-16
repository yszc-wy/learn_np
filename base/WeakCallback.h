/*
* Author:           yszc-wy@foxmail.com 
* Encoding:			    utf-8
* Description:      弱回调封装
*/
#pragma once

// C系统库头文件

// C++系统库头文件
#include <functional>
#include <memory>
// 第三方库头文件

// 本项目头文件


namespace yszc
{

template <typename CLASS,typename... ARGS> 
class WeakCallback
{
 public:
  WeakCallback(const std::weak_ptr<CLASS>& obj,
               const std::function<void (CLASS*,ARGS...)>& function)
    : object_(obj),
      function_(function)
  {
  }
  void operator() (ARGS&&... args)
  {
    std::shared_ptr<CLASS> ptr(object_.lock());
    if (ptr)
    {
      function_(ptr.get(), std::forward<ARGS>(args)...);
    }
  }
 private:
  std::weak_ptr<CLASS> object_;
  std::function<void (CLASS*, ARGS...)> function_;
};

template<typename CLASS, typename... ARGS>
WeakCallback<CLASS, ARGS...> makeWeakCallback(const std::shared_ptr<CLASS>& object,
                                              void (CLASS::*function)(ARGS...))
{
  return WeakCallback<CLASS,ARGS...>(object,function);
}

template<typename CLASS, typename... ARGS>
WeakCallback<CLASS, ARGS...> makeWeakCallback(const std::shared_ptr<CLASS>& object,
                                              void (CLASS::*function)(ARGS...) const)
{
  return WeakCallback<CLASS,ARGS...>(object,function);j
}

}

