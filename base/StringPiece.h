/*
* Encoding:			    utf-8
* Description:      表示字符串的方便工具类,主要方便在可以从C字符串中构造,可以从string中构造,且拷贝代价小
* 和leveldb中的slice差不多
* 这种slice要保证其对象生命周期比数据短,不然会访问到空指针
*/
#pragma once

// C系统库头文件
#include <string.h>
// C++系统库头文件
#include <iosfwd>    // for ostream forward-declaration
// 第三方库头文件

// 本项目头文件
#include "base/Types.h"



// 系统结构体或其他命名空间中类的前向声明(注意要包含相应的命名空间)


namespace yszc
{

// For passing C-style string argument to a function.
class StringArg // copyable
{
 public:
  StringArg(const char* str)
    : str_(str)
  { }

  StringArg(const string& str)
    : str_(str.c_str())
  { }

  const char* c_str() const { return str_; }

 private:
  const char* str_;
};

class StringPiece
{
 private:
  const char*   ptr_;
  int           length_;
 public:
  // 我们这里提供了non-explicit singleton constructors 
  // 这样用户在将const char 或 string 赋值给StringPiece类型时会自动转换
  StringPiece()
    : ptr_(NULL), length_(0) { }
  StringPiece(const char* str)
    : ptr_(str), length_(static_cast<int>(strlen(ptr_))) { }
  StringPiece(const unsigned char* str)
    : ptr_(reinterpret_cast<const char*>(str)),
      length_(static_cast<int>(strlen(ptr_))) { }
  StringPiece(const string& str)
    : ptr_(str.data()), length_(static_cast<int>(str.size())) { }
  StringPiece(const char* offset, int len)
    : ptr_(offset), length_(len) { }


  const char* data() const { return ptr_; }
  int size() const { return length_; }
  bool empty() const { return length_ == 0; }
  const char* begin() const { return ptr_; }
  const char* end() const { return ptr_ + length_; }

  void clear() { ptr_ = NULL; length_ = 0; }
  void set(const char* buffer, int len) { ptr_ = buffer; length_ = len; }
  void set(const char* str) {
    ptr_ = str;
    length_ = static_cast<int>(strlen(str));
  }
  void set(const void* buffer, int len) {
    ptr_ = reinterpret_cast<const char*>(buffer);
    length_ = len;
  }

  char operator[](int i) const { return ptr_[i]; }

  void remove_prefix(int n) {
    ptr_ += n;
    length_ -= n;
  }

  void remove_suffix(int n) {
    length_ -= n;
  }

  bool operator==(const StringPiece& x) const {
    return ((length_ == x.length_) &&
            (memcmp(ptr_, x.ptr_, length_) == 0));
  }
  bool operator!=(const StringPiece& x) const {
    return !(*this == x);
  }


  // 定义比较符号的方便写法
#define STRINGPIECE_BINARY_PREDICATE(cmp,auxcmp)                             \
  bool operator cmp (const StringPiece& x) const {                           \
    int r = memcmp(ptr_, x.ptr_, length_ < x.length_ ? length_ : x.length_); \
    return ((r auxcmp 0) || ((r == 0) && (length_ cmp x.length_)));          \
  }
  STRINGPIECE_BINARY_PREDICATE(<,  <);
  STRINGPIECE_BINARY_PREDICATE(<=, <);
  STRINGPIECE_BINARY_PREDICATE(>=, >);
  STRINGPIECE_BINARY_PREDICATE(>,  >);
#undef STRINGPIECE_BINARY_PREDICATE


  int compare(const StringPiece& x) const {
    int r = memcmp(ptr_, x.ptr_, length_ < x.length_ ? length_ : x.length_);
    if (r == 0) {
      if (length_ < x.length_) r = -1;
      else if (length_ > x.length_) r = +1;
    }
    return r;
  }

  string as_string() const {
    return string(data(), size());
  }

  void CopyToString(string* target) const {
    target->assign(ptr_, length_);
  }

  // Does "this" start with "x"
  bool starts_with(const StringPiece& x) const {
    return ((length_ >= x.length_) && (memcmp(ptr_, x.ptr_, x.length_) == 0));
  }
};

}

// 为了让StringPiece类型可以更好的适配STL库

#ifdef HAVE_TYPE_TRAITS
// This makes vector<StringPiece> really fast for some STL implementations
// 没有显示的构造函数,可以直接执行内存拷贝
template<> struct __type_traits<yszc::StringPiece> {
  typedef __true_type    has_trivial_default_constructor;
  typedef __true_type    has_trivial_copy_constructor;
  typedef __true_type    has_trivial_assignment_operator;
  typedef __true_type    has_trivial_destructor;
  typedef __true_type    is_POD_type;
};
#endif


// allow StringPiece to be logged
std::ostream& operator<<(std::ostream& o, const yszc::StringPiece& piece);

