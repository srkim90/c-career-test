#ifndef CAREER_TEST2_LOGGER_H
#define CAREER_TEST2_LOGGER_H

#endif //CAREER_TEST2_LOGGER_H

typedef enum {
    LOG_NONE = 0,
    LOG_CRT,
    LOG_MAJ,
    LOG_MIN,
    LOG_INF,
    LOG_DBG,
    LOG_TRC
} LOG_LEVEL;

#define C_NRML "\033[0m"
#define C_BLCK "\033[30m"
#define C_RED  "\033[31m"
#define C_GREN "\033[32m"
#define C_YLLW "\033[33m"
#define C_BLUE "\033[34m"
#define C_PRPL "\033[35m"
#define C_AQUA "\033[36m"

#define MAX_LOCAL_TIME 64
#define MAX_LOG_SIZE 1024
extern void setLogLevel(int log_lv);
extern void smtpLog (int log_lv, int line, const char * file_name, const char * fmttxt, ... );
#define LOG(l,f,a...) smtpLog(l, __LINE__, __FILE__, f,## a )