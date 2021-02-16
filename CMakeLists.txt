# cmake 最低版本号要求
cmake_minimum_required(VERSION 2.8)

# 设置项目名和版本号
project(web_server C CXX)


# 没在外部设置BUILD_TYPE的情况下默认为Release
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

set(CXX_FLAGS
 -g
 # -DVALGRIND
 -DCHECK_PTHREAD_RETURN_VALUE
 -D_FILE_OFFSET_BITS=64
 -Wall
 -Wextra
 -Werror
 -Wconversion
 -Wno-unused-parameter
 -Wold-style-cast
 -Woverloaded-virtual
 -Wpointer-arith
 -Wshadow
 -Wwrite-strings
 -march=native
 # -MMD
 -std=c++11
 -rdynamic
 )
if(CMAKE_BUILD_BITS EQUAL 32)
  list(APPEND CXX_FLAGS "-m32")
endif()
# 查看CMAKE使用的编译器
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  list(APPEND CXX_FLAGS "-Wno-null-dereference")
  list(APPEND CXX_FLAGS "-Wno-sign-conversion")
  list(APPEND CXX_FLAGS "-Wno-unused-local-typedef")
  list(APPEND CXX_FLAGS "-Wthread-safety")
  list(REMOVE_ITEM CXX_FLAGS "-rdynamic")
endif()
# 为CMAKE_CXX_FLAGS添加额外的自定义FLAGS
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")


# 指定CMAKE默认的CXX编译全局变量
# 设置debug和release等级
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
# PROJECT_BINARY_DIR时cmake执行时所在的当前目录(build)
# cmake执行后会按照项目的结构生成一个文件树(根据add_subdir所包含的其他Cmakelists.txt文件)
# 设置可执行文件输出路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
# 设置库文件输出路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
# 输出项目路径
message(STATUS "PROJECT_BINARY_DIR = ${PROJECT_BINARY_DIR}.")

# 查找web_server需要的库文件和包
# find_package相关库文件和package进行查找,通过使用这些函数就不必手动定位各种库的include path和library
# find_package会自动配置好:
# <NAME>_FOUND
# <NAME>_INCLUDE_DIRS
# <NAME>_LIBRARIES
# <NAME>_DEFINITIONS
# web_server_net等子文件在进行target_link时都没有包含boost,是因为boost基本上是以头文件形式被引用,无需链接
# find_package请和include_directories(${OpenCV_INCLUDE_DIRS}) target_link_libraries( test ${OpenCV_LIBS})配合食用，三步走
message(STATUS "CMAKE_MODULE_PATH = ${CMAKE_MODULE_PATH}.")
message(STATUS "CMAKE_DEFAULT_INCLUDE_PATH = ${CMAKE_INCLUDE_PATH}.")
find_package(Boost REQUIRED)
find_package(Protobuf)
find_package(CURL)
find_package(ZLIB)
# 查找头文件:
# 1.从CMAKE_INCLUDE_PATH中查找
# 2.从系统默认头目录/usr/include、/usr/local/include中查找
find_path(CARES_INCLUDE_DIR ares.h)
find_library(CARES_LIBRARY NAMES cares)
find_path(MHD_INCLUDE_DIR microhttpd.h)
find_library(MHD_LIBRARY NAMES microhttpd)
find_library(BOOSTTEST_LIBRARY NAMES boost_unit_test_framework)
find_library(BOOSTPO_LIBRARY NAMES boost_program_options)
find_library(BOOSTSYSTEM_LIBRARY NAMES boost_system)
find_path(TCMALLOC_INCLUDE_DIR gperftools/heap-profiler.h)
find_library(TCMALLOC_LIBRARY NAMES tcmalloc_and_profiler)
find_path(HIREDIS_INCLUDE_DIR hiredis/hiredis.h)
find_library(HIREDIS_LIBRARY NAMES hiredis)
find_path(GD_INCLUDE_DIR gd.h)
find_library(GD_LIBRARY NAMES gd)
find_program(THRIFT_COMPILER thrift)
find_path(THRIFT_INCLUDE_DIR thrift)
find_library(THRIFT_LIBRARY NAMES thrift)

# 输出查找结果
if(CARES_INCLUDE_DIR AND CARES_LIBRARY)
  message(STATUS "found cares")
endif()
if(CURL_FOUND)
  message(STATUS "found curl")
endif()
if(PROTOBUF_FOUND)
  message(STATUS "found protobuf")
endif()
if(TCMALLOC_INCLUDE_DIR AND TCMALLOC_LIBRARY)
  message(STATUS "found tcmalloc")
endif()
if(ZLIB_FOUND)
  message(STATUS "found zlib")
endif()
if(HIREDIS_INCLUDE_DIR AND HIREDIS_LIBRARY)
  message(STATUS "found hiredis")
endif()
if(GD_INCLUDE_DIR AND GD_LIBRARY)
  message(STATUS "found gd")
endif()
if(THRIFT_COMPILER AND THRIFT_INCLUDE_DIR AND THRIFT_LIBRARY)
  message(STATUS "found thrift")
endif()


# 添加muduo库的头文件
# include_directories($ENV{MUDUO_INCLUDE})
# 添加muduo库的库文件
# link_directories($ENV{MUDUO_LIB})
# 添加boost的头文件
include_directories(${Boost_INCLUDE_DIRS})
# 添加项目头文件
message(STATUS "PROJECT_SOURCE_DIR = ${PROJECT_SOURCE_DIR}")
include_directories(${PROJECT_SOURCE_DIR})

# 添加子目录
add_subdirectory(base)
add_subdirectory(net)
# 练习用子目录
add_subdirectory(train)
# web server用子目录
add_subdirectory(server)


# 生成主文件
# add_executable(main main.cc)

# 这里才是真正链接库文件的地方,所有输入的库以库的绝对路径或库名称表示(库名称会在之前添加的库搜索目录中查找),MUDUO_LIBS实际上是绝对路径的集合
# 就算库文件在/usr/lib/(link_directories)里,不在此链接也无法编译通过
# 动态库(.a)和静态库(.so)都可以链接
# 链接顺序:A 依赖 B,则A应该放在B左边
# main 依赖 libmuduo_net和libmuduo_base, libmuduo_net 依赖 libmuduo_base,因此有下面的依赖顺序,为了方便写库名,我们使用link_directories添加库路径
# target_link_libraries(main web_server_net)


# 如果在内核库函数头文件中不用extern“C”来声明库函数的话，在编写C++应用程序时，包含库头文件，在C++文件链接时就会以C++标准来链接库的函数名，
# 而在库文件实现时是用C来实现的，二者函数名不同，在链接时就会出现找不到函数的现象。
# undefined reference to `muduo::CurrentThread::t_cachedTid' 中undefined表示你根本就没有链接头文件
# 出现错误先用clang调试,调好了再用cmake
# 库的链接顺序不当也会导致undefine reference error! https://www.jianshu.com/p/e6854492e401
# boost库和stl库无法自动补全,且cmake无法让/usr/include添加到compile_commands中,所以直接修改compile_commands.json文件,来让ycm补全回复,另外把cmm命令和cmmrun命令的cp compile_commands取消,来防止文件被覆盖
# 改进上述方法,使用python的json库将include path 写到compile_commands.json中
# 如果编译多个文件,会在compiles_json中生成针对多个cc文件的flag,导致在编辑head文件时出现无法补全的问题,这时把除了main文件以外的其他flag全部删除,或者使用add_path.py向每个编译单元的flag中添加默认路径
