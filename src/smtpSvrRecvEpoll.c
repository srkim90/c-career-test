#include "main.h"
#include <sys/epoll.h>

#define EPOLL_SIZE 1024
int epoll_fd;
pthread_mutex_t g_epoll_lock = PTHREAD_MUTEX_INITIALIZER;

void smtpWaitAsync(int server_fd) {
    int event_count;
    struct epoll_event init_event;
    epoll_fd = epoll_create(EPOLL_SIZE);
    struct epoll_event *events = malloc(sizeof(struct epoll_event) * EPOLL_SIZE);

    memset(&init_event, 0x00, sizeof(struct epoll_event));
    init_event.events = EPOLLIN;
    init_event.data.fd = server_fd;
    smtp_session_t *session = NULL;

    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &init_event);

    while (!g_sys_close) {
        event_count = epoll_wait(epoll_fd, events, EPOLL_SIZE, -1);

        for (int i = 0; i < event_count; ++i) {

            if (events[i].data.fd == server_fd) {
                if ((session = smtpHandleInboundConnection(server_fd)) == NULL) {
                    break;
                }
                init_event.events = EPOLLIN;
                init_event.data.fd = session->sock_fd;
                init_event.data.ptr = (void *) session;
                pthread_mutex_lock ( &g_epoll_lock ) ;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, session->sock_fd, &init_event);
                pthread_mutex_unlock ( &g_epoll_lock ) ;
                LOG (LOG_INF, "%s : %sSMTP Connection created%s : fd = %d, session_id=%s\n", __func__, C_YLLW, C_NRML,
                     session->sock_fd, session->session_id);
                sendGreetingMessage(session);
            } else {
                session = events[i].data.ptr;
                pthread_mutex_lock ( &g_epoll_lock ) ;
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, session->sock_fd, NULL);
                pthread_mutex_unlock ( &g_epoll_lock ) ;
                itcqPutSession(session);
            }
        }
    }
    close(server_fd);
    close(epoll_fd);
}

void *H_SERVER_EPOLL_WORK_TH(void *args) {
    int nLine, nErr;
    char buf[MAX_BUF_SIZE];
    smtp_session_t *session;
    struct epoll_event init_event;

    memset(&init_event, 0x00, sizeof(struct epoll_event));

    while (!g_sys_close) {
        session = itcqGetSession();
        if (session == NULL) {
            msleep(25);
            continue;
        }

        if ((nLine = smtpReadLine(session->sock_fd, buf, sizeof(buf))) <= 0) {
            LOG (LOG_INF, "%s : %sSMTP Connection closed%s : fd = %d, session_id=%s\n", __func__, C_YLLW, C_NRML, session->sock_fd,
                 session->session_id);
            delSmtpSession(session->session_id);
            continue;
        }

        if ((nErr = doSmtpDispatch(session, buf)) != SMTP_DISPATCH_OK) {
            if (nErr == SMTP_DISPATCH_FAIL) {
                LOG(LOG_INF, "Smtp connection close by error!");
            }
            continue;
        }
        init_event.events = EPOLLIN;
        init_event.data.fd = session->sock_fd;
        init_event.data.ptr = (void *) session;

        pthread_mutex_lock ( &g_epoll_lock ) ;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, session->sock_fd, &init_event);
        pthread_mutex_unlock ( &g_epoll_lock ) ;

    }
    return NULL;
}


int smtpStartWorkThreads(int n_work_threads) {
    int nErr;
    pthread_t clientTh;
    pthread_attr_t clientThAttr;
    pthread_attr_init(&clientThAttr);

    nErr = pthread_attr_setstacksize(&clientThAttr, (10 * 1024 * 1024));

    for (int i = 0; i < n_work_threads; i++) {
        if ((nErr = pthread_create(&clientTh, &clientThAttr, H_SERVER_EPOLL_WORK_TH, NULL)) < 0) {
            LOG (LOG_MAJ, "Err. Worker Thread Create Failed. Err.= '%s', idx=%d\n", strerror(nErr), i);
            return -1;
        }
    }

    return 0;
}