#include "main.h"

static char* getLocalTime(char * buf,size_t sz_buf)
{
    struct tm t;
    time_t timer = time(NULL);
    localtime_r(&timer, &t);
    snprintf(buf, sz_buf, "[%d-%02d-%02d %02d:%02d:%02d]", t.tm_year + 1900, t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);

    return buf;
}
int gLogLevel = LOG_INF;
void setLogLevel(int log_lv) {
    if (log_lv >= LOG_NONE && log_lv <= LOG_TRC) {
        gLogLevel = log_lv;
        return;
    }
}

char * getFileName(const char * fullPath) {
    int wp_idx = 0;
    for (int i=0;fullPath[i];i++) {
        if (fullPath[i] == '/') {
            wp_idx = i + 1;
        }
    }
    return (char *)fullPath + wp_idx;
}

void smtpLog (int log_lv, int line, const char * file_name, const char * fmttxt, ... )
{
    char buf[MAX_LOG_SIZE] = {0,};
    char local_time[MAX_LOCAL_TIME] = {0,};

    // check now log level
    if (log_lv > gLogLevel) {
        return;
    }
    va_list ap;

    va_start(ap, fmttxt);
    vsprintf(buf, fmttxt, ap);
    va_end(ap);
    for (int i =0;buf[i];i++) {
        char c = buf[i];
        if (c == '\r') {
            buf[i] = ' ';
        }
    }
    if (buf[strlen(buf)-1] != '\n') {
        strcat(buf, "\n");
    }

    printf("%s[%-20s:%-6d]: %s", getLocalTime(local_time, sizeof(local_time)), getFileName(file_name), line, buf);
}
