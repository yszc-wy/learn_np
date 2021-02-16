/*
* Author:           yszc-wy@foxmail.com 
* Encoding:			    utf-8
* Description:      作为一个标志,表示继承的类是可拷贝的
*/
#pragma once


namespace yszc
{

class copyable
{
 protected:
  copyable() = default;
  ~copyable() = default;
};

}

