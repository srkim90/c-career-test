#ifndef CAREER_TEST3_MAIN_H
#define CAREER_TEST3_MAIN_H

#include <stdint-gcc.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <malloc.h>

#include "logger.h"
#include "smtpData.h"
#include "smtpSession.h"
#include "smtpSock.h"
#include "smtpSvrRecvFdSet.h"
#include "smtpUtils.h"
#include "smtpDispatch.h"
#include "smtpMailFile.h"
#include "smtpSvrRecvEpoll.h"
#include "smtpItcq.h"

extern int g_sys_close;

#endif //CAREER_TEST3_MAIN_H
