#include "main.h"

int smtpAcceptSock(int server_fd) {
    struct sockaddr_in sock_addr_in;

    int client_fd;
    int addrLen = sizeof(sock_addr_in);

    client_fd = accept(server_fd, (struct sockaddr *) &sock_addr_in, (socklen_t *) &addrLen);

    LOG (LOG_TRC, "TCP Socket Accepted < new client : %d > \n", client_fd);

    if (client_fd < 0) {
        LOG (LOG_MAJ, "%s : Err. accept() Failed. server_fd = %d , Err is ( %d , %s )\n", __func__, server_fd, errno,
             strerror(errno));
        return -1;
    }

    return client_fd;
}

smtp_session_t *smtpHandleInboundConnection(int server_fd) {
    int client_fd;
    smtp_session_t *session;
    client_fd = smtpAcceptSock(server_fd);

    if (client_fd < 0) {
        LOG (LOG_CRT, "Err. smtpAcceptSock() Failed. server_fd = %d\n", server_fd);
        client_fd = -1;
        return NULL;
    }

    session = (smtp_session_t *) malloc(sizeof(smtp_session_t));
    if (NULL == session) {
        close(client_fd);
        return NULL;
    }

    session->sock_fd = client_fd;
    session->port_num = smtpGetPeerPortNum(client_fd);
    smtpGetPeerIP4Addr(client_fd, session->str_ipv4);
    smtpSetSessionId(session);

    if (addSmtpSession(session) == NULL) {
        LOG (LOG_MAJ, "Err. Fail to make smtp session\n");
        close(client_fd);
        return NULL;
    }

    return session;
}

int smtpServerOpen(uint16_t n_port) {
    int sock_fd;
    int n_ret = 0;
    int n_opt = 0;
    int n_reuse = 1;
    struct linger opt_linger;
    struct sockaddr_in sock_in_addr;

    /* 1. Create Server Socket				*/
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        LOG(LOG_INF, "Err.  socket() Failed. \n");
        return (-1);
    }

    /* 2. Set socket option : Reuse Address */
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (char *) &n_reuse, sizeof(int)) < 0) {
        LOG(LOG_INF, "Err.  setsockopt() : Reuse Err. n_ret = %d\n", n_ret);
        close(sock_fd);
        return (-1);
    }

    /* 3. Set Socket Option : Linger : When Socket Closed, Prevent Time_Wait Status of Socket   */
    opt_linger.l_onoff = 1;
    opt_linger.l_linger = 0;

    if (setsockopt(sock_fd, SOL_SOCKET, SO_LINGER, (char *) &opt_linger, sizeof(opt_linger)) < 0) {
        LOG(LOG_INF, "Err.  setsockopt() : Linger Err. n_ret = %d\n", n_ret);
        close(sock_fd);
        return (-1);
    }

    /* 4. Set Socket Option : Re-Use Port	*/
    n_opt = 1;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, &n_opt, sizeof(n_opt)) < 0) {
        close(sock_fd);
        return (-1);
    }

    /* 5. Set Socket Option : Recv. Socket Buffer	*/
    n_opt = 8 * 1024 * 1024;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_RCVBUF, &n_opt, sizeof(n_opt)) < 0) {
        LOG(LOG_INF, "Err. Setsockopt() : Set Receive Socket Buffer Failed. \n");
        close(sock_fd);
        return (-1);
    }

    /* 6. Set Socket Option : Send. Socket Buffer	*/
    if (setsockopt(sock_fd, SOL_SOCKET, SO_SNDBUF, &n_opt, sizeof(n_opt)) < 0) {
        LOG(LOG_INF, "Err. Setsockopt() : Set Send Socket Buffer Failed. \n");
        close(sock_fd);
        return (-1);
    }

    /* 7. Binding Socket    */
    sock_in_addr.sin_family = AF_INET;
    sock_in_addr.sin_port = htons(n_port);
    sock_in_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock_fd, (struct sockaddr *) &sock_in_addr, sizeof(sock_in_addr))) {
        LOG(LOG_INF, "Err.  bind() Err. n_ret = %d\n", n_ret);
        close(sock_fd);
        return (-1);
    }

    /* 8. Listen Socket		*/
    if (listen(sock_fd, MAX_TCP_LISTEN) < 0) {
        LOG(LOG_INF, "Err.  listen() Err. n_ret = %d\n", n_ret);
        close(sock_fd);
        return (-1);
    }

    return sock_fd;
}

size_t smtpReadLine(int sock_fd, char *p_data, size_t sz_buf) {
    char *wp = NULL;
    size_t n_byte;
    size_t n_byte_read = 0;
    memset(p_data, 0x00, sz_buf);
    wp = (char *) p_data;

    while (n_byte_read < sz_buf - 1) {
        n_byte = read(sock_fd, wp, 1);
        n_byte_read = wp - (char *) p_data;
        if (n_byte <= 0) {
            if (!n_byte) break; /* EOF */

            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                continue;
            } else {
                LOG (LOG_MAJ, "%s : Err. read error. errno=%d, sErr=%s\n", __func__, errno, strerror(errno));
                return (-1);
            }
        }

        if (*wp == '\n') {
            *(wp + 1) = 0;
            break;
        }
        wp += n_byte;
    }

    return wp - (char *) p_data;
}

int smtpSendData(int sock_fd, void *p_data, size_t n_length) {
    char *wp = NULL;

    LOG(LOG_DBG, "%ssend%s --> %s", C_RED, C_NRML, (char *)p_data);

    size_t n_left;
    ssize_t n_written;

    int send_count = 0;
    int retry_count = 0;

    wp = (char *) p_data;
    n_left = n_length;

    while (n_left > 0) {
        n_written = send(sock_fd, wp, n_left, MSG_NOSIGNAL);

        if (n_written <= 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN || errno == ENOBUFS || errno == EINTR) {
                if (retry_count++ > 10) {
                    LOG (LOG_MAJ, "%s : Err. Retry Error. error=%d, sErr=%s\n", __func__, errno, strerror(errno));
                    return (-1);
                }
                continue;
            }

            LOG (LOG_MAJ, "%s : Err. send error. errno=%d, sErr=%s\n", __func__, errno, strerror(errno));
            return (-1);
        }

        n_left -= n_written;
        wp += n_written;
        send_count += n_written;
    }

    return send_count;
}

int smtpGetPeerIP4Addr(int sock_fd, char *str_ipv4) {
    struct sockaddr peer;
    struct sockaddr_in *peer_in = (struct sockaddr_in *) &peer;
    int peerlen = sizeof(peer);

    getpeername(sock_fd, &peer, (socklen_t *) &peerlen);
    sprintf(str_ipv4, "%s", inet_ntoa(peer_in->sin_addr));
    return 0;
}

int smtpGetPeerPortNum(int sock_fd) {
    struct sockaddr peer;
    int peer_len = sizeof(peer);
    struct sockaddr_in *peer_in = (struct sockaddr_in *) &peer;
    getpeername(sock_fd, &peer, (socklen_t *) &peer_len);
    return (int) ((unsigned int) peer_in->sin_port);
}

char *smtpMakeSessionId(char *session_id, char *ls_ipv4, int ln_port, int sock_fd) {
    sprintf(session_id, "sid:%s:%d:%d", ls_ipv4, ln_port, sock_fd);
    return session_id;
}

void smtpSetSessionId(smtp_session_t *session) {
    smtpMakeSessionId(session->session_id, session->str_ipv4, session->port_num, session->sock_fd);
}