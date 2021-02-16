#!/bin/sh
# 在shell中解决compile_commands的问题
set -x
# 获取当前文件夹路径
SOURCE_DIR=`pwd` 
# build_dir中包含build文件夹和install文件夹,文件名制定了编译类型
BUILD_DIR=${BUILD_DIR:-./build}
BUILD_TYPE=${BUILD_TYPE:-release}
INSTALL_DIR=${INSTALL_DIR:-../${BUILD_TYPE}-install-cpp11}
CXX=${CXX:-g++}
COMPILE_COMMAND_FIX=$FIX_COMPILE_COMMAND

# 在项目目录中建立指向构建目录中json的符号链接,这样Youcomplete就可以使用了
ln -sf $BUILD_DIR/$BUILD_TYPE-cpp11/compile_commands.json

# 构建并进入build目录
# cmake会在当前目录下生成build文件
# 指定release
# 将install目录指定给cmake
# 导出编译指令
# CMakeLists.txt 所在的文件夹
# $*表示传递给脚本的所有参数
# '\'后面不能有空格
mkdir -p $BUILD_DIR/$BUILD_TYPE-cpp11 \
  && cd $BUILD_DIR/$BUILD_TYPE-cpp11 \
  && mkdir -p $INSTALL_DIR \
  && cmake \
           -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
           -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
           -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
           $SOURCE_DIR \
  && make $* \
  && python $COMPILE_COMMAND_FIX

# Use the following command to run all the unit tests
# at the dir $BUILD_DIR/$BUILD_TYPE :
# CTEST_OUTPUT_ON_FAILURE=TRUE make test

# cd $SOURCE_DIR && doxygen

