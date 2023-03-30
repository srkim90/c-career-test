cmake_minimum_required(VERSION 3.24)
project(career_test3 C)

set(CMAKE_C_STANDARD 99)
include_directories(./include ../include)
add_executable(career_test3 main.c src/logger.c include/logger.h src/smtpSock.c include/smtpSock.h include/smtpData.h src/smtpSvrRecvFdSet.c include/smtpSvrRecvFdSet.h src/smtpUtils.c include/smtpUtils.h src/smtpSession.c include/smtpSession.h include/main.h src/smtpDispatch.c include/smtpDispatch.h src/smtpMailFile.c include/smtpMailFile.h src/smtpSvrRecvEpoll.c include/smtpSvrRecvEpoll.h src/smtpItcq.c include/smtpItcq.h src/smtpTps.c)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(career_test3 PRIVATE Threads::Threads)
