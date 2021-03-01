/*
* Author:           
* Encoding:			    utf-8
* Description:      
*/
#pragma once

// C系统库头文件

// C++系统库头文件
#include <stdint.h>
// 第三方库头文件

// 本项目头文件
#include "base/noncopyable.h"



// 系统结构体或其他命名空间中类的前向声明(注意要包含相应的命名空间)


namespace yszc
{

namespace detail
{

template <typename T>
class AtomicIntegerT :public noncopyable
{
 public:
  AtomicIntegerT()
    : value_(0)
  {
  }

  T get()
  {
    // in gcc >= 4.7: __atomic_load_n(&value_, __ATOMIC_SEQ_CST)
    return __sync_val_compare_and_swap(&value_, 0, 0);
  }

  // value_+x->value,且返回之前的值
  // 类似于i++
  T getAndAdd(T x)
  {
    // in gcc >= 4.7: __atomic_fetch_add(&value_, x, __ATOMIC_SEQ_CST)
    return __sync_fetch_and_add(&value_, x);
  }

  // value_+x->value,且返回累加之后的值
  // 类似于++i
  T addAndGet(T x)
  {
    return getAndAdd(x) + x;
  }

  T incrementAndGet()
  {
    return addAndGet(1);
  }

  T decrementAndGet()
  {
    return addAndGet(-1);
  }

  void add(T x)
  {
    getAndAdd(x);
  }

  // 这两个函数有效果吗,使用addAndGet貌似没有效果啊
  void increment()
  {
    incrementAndGet();
  }

  void decrement()
  {
    decrementAndGet();
  }

  T getAndSet(T newValue)
  {
    // in gcc >= 4.7: __atomic_exchange_n(&value_, newValue, __ATOMIC_SEQ_CST)
    return __sync_lock_test_and_set(&value_, newValue);
  }
 private:
  volatile T value_;
};

} // namespace detail

typedef detail::AtomicIntegerT<int32_t> AtomicInt32;
typedef detail::AtomicIntegerT<int64_t> AtomicInt64;
} // namespace yszc

