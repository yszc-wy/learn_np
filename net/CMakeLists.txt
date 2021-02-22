set(net_SRCS
  EventLoop.cc            
  EventLoopThread.cc      
  EventLoopThreadPool.cc  
  Poller.cc
  poller/DefaultPoller.cc
  poller/EPollPoller.cc
  poller/PollPoller.cc
  Channel.cc    
  TimerQueue.cc
  Timer.cc
  Acceptor.cc
  Buffer.cc     
  Socket.cc         
  SocketsOps.cc     
  InetAddress.cc          
  TcpConnection.cc
  Connector.cc  
  TcpServer.cc
  TcpClient.cc
  TimeWheel.cc
  )

add_library(web_server_net ${net_SRCS})
target_link_libraries(web_server_net web_server_base)



install(TARGETS web_server_net DESTINATION lib)


set(HEADERS
  Buffer.h
  Callbacks.h
  Channel.h
  Endian.h
  EventLoop.h
  EventLoopThread.h
  EventLoopThreadPool.h
  InetAddress.h
  TcpClient.h
  TcpConnection.h
  TcpServer.h
  TimerId.h
  TimeWheel.h
  )
install(FILES ${HEADERS} DESTINATION include/net)

# add_subdirectory(poller)

add_subdirectory(protobuf)
add_subdirectory(http)


