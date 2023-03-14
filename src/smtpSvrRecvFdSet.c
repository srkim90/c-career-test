#include "main.h"

void smtpWaitSync(int server_fd) {
    int n_err;

    smtp_session_t *session = NULL;

    pthread_t client_th;
    pthread_attr_t client_th_attr;
    pthread_attr_init(&client_th_attr);

    n_err = pthread_attr_setstacksize(&client_th_attr, (10 * 1024 * 1024));

    while (!g_sys_close) {

        if ((session = smtpHandleInboundConnection(server_fd)) == NULL) {
            break;
        }

        if ((n_err = pthread_create(&client_th, &client_th_attr, H_SERVER_RECV_FDSET_TH, (void *) (session))) < 0) {
            LOG (LOG_MAJ, "Err. New Client Thread Create Failed. Err.= '%s'\n", strerror(n_err));
            delSmtpSession(session->session_id);
            msleep(100);
            continue;
        }
    }
    close(server_fd);
}

void *H_SERVER_RECV_FDSET_TH(void *args) {
    size_t nLine;
    fd_set read_fd_set;
    char buf[MAX_BUF_SIZE];
    struct timeval time_value;
    int sock_fd, max_fd, n_err;
    smtp_session_t *session = (smtp_session_t *) args;
    pthread_detach(pthread_self());

    sock_fd = session->sock_fd;
    LOG (LOG_INF, "%s : %sSMTP Connection created%s : fd = %d, session_id=%s\n", __func__, C_YLLW, C_NRML, sock_fd, session->session_id);
    sendGreetingMessage(session);
    while (!g_sys_close) {
        FD_ZERO (&read_fd_set);
        FD_SET  (sock_fd, &read_fd_set);
        max_fd = sock_fd;
        time_value.tv_sec = 1;
        time_value.tv_usec = 0;
        n_err = select(max_fd + 1, &read_fd_set, NULL, NULL, &time_value);

        if (n_err < 0) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) continue;
            LOG (LOG_MAJ, "%s : Err. select failed. error = %s\n", __func__, strerror(errno));
            break;
        }
        if ((nLine = smtpReadLine(sock_fd, buf, sizeof(buf))) <= 0) {
            break;
        }

        if ((n_err = doSmtpDispatch(session, buf)) != SMTP_DISPATCH_OK) {
            if (n_err == SMTP_DISPATCH_FAIL) {
                LOG(LOG_INF, "Smtp connection close by error!");
            }
            break;
        }
    }

    LOG (LOG_INF, "%s : SMTP Connection closed : fd = %d, session_id=%s\n", __func__, sock_fd, session->session_id);
    delSmtpSession(session->session_id);
    return NULL;
}