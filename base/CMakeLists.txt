# 在每个源文件目录中
set(base_SRCS  # 收集源文件
  AsyncLogging.cc
  Condition.cc
  CountDownLatch.cc
  CurrentThread.cc
  Date.cc
  Exception.cc
  FileUtil.cc
  LogFile.cc
  Logging.cc
  LogStream.cc
  ProcessInfo.cc
  Timestamp.cc
  Thread.cc
  ThreadPool.cc
  TimeZone.cc
  )

# 构建库文件
add_library(web_server_base ${base_SRCS})
# 将库文件链接到依赖库中
target_link_libraries(web_server_base pthread rt)

install(TARGETS web_server_base DESTINATION lib)

file(GLOB HEADERS "*.h")
install(FILES ${HEADERS} DESTINATION include/base)

