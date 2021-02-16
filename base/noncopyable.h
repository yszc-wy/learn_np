/*
* Author:           yszc-wy@foxmail.com 
* Encoding:			    utf-8
* Description:      对象类型一般禁止拷贝
*/
#pragma once

// C系统库头文件

// C++系统库头文件

// 第三方库头文件

// 本项目头文件




// 系统结构体或其他命名空间中类的前向声明(注意要包含相应的命名空间)


namespace yszc
{

class noncopyable
{
 public:
  noncopyable(const noncopyable& cp)=delete;
  void operator=(const noncopyable& cp)=delete;
 protected:
  noncopyable() = default;
  ~noncopyable() = default;
};

}

