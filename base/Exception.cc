
/*
* Encoding:			    utf-8
* Description:      
*/

// 本类头文件

// C系统库头文件

// C++系统库头文件

// 第三方库头文件

// 本项目头文件


#include "base/Exception.h"
#include "base/CurrentThread.h"

namespace yszc
{

Exception::Exception(string msg)
  : message_(std::move(msg)),
    stack_(CurrentThread::stackTrace(/*demangle=*/false))
{
}

}  // namespace yszc
