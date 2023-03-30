#include "main.h"

pthread_mutex_t g_tps_lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct smtp_tps {
    int     n_tcp_connect;
    int     n_tcp_close;
    int     n_smtp_message_send;
    int     n_smtp_message_recv;
} smtp_tps_t;
smtp_tps_t g_tps;

void inc_smtp_tps_tcp_connect() {
    pthread_mutex_lock(&g_tps_lock);
    g_tps.n_tcp_connect += 1;
    pthread_mutex_unlock(&g_tps_lock);
}

void inc_smtp_tps_tcp_close() {
    pthread_mutex_lock(&g_tps_lock);
    g_tps.n_tcp_close += 1;
    pthread_mutex_unlock(&g_tps_lock);
}

void inc_smtp_message_send() {
    pthread_mutex_lock(&g_tps_lock);
    g_tps.n_smtp_message_send += 1;
    pthread_mutex_unlock(&g_tps_lock);
}

void inc_smtp_message_recv() {
    pthread_mutex_lock(&g_tps_lock);
    g_tps.n_smtp_message_recv += 1;
    pthread_mutex_unlock(&g_tps_lock);
}


void *SMTP_TPS_TH(void *args) {
    smtp_tps_t old_tps;
    smtp_tps_t local_tps;

    while (!g_sys_close) {
        sleep(1);
        pthread_mutex_lock(&g_tps_lock);
        local_tps = g_tps;
        pthread_mutex_unlock(&g_tps_lock);

        LOG(LOG_INF, "SMTP Server Traffic: %sn_tcp_connect%s=%s%d%s(+%s%d%s), %sn_tcp_close%s=%s%d%s(+%s%d%s), %sn_smtp_message_send%s=%s%d%s(+%s%d%s), %sn_smtp_message_recv%s=%s%d%s(+%s%d%s)", 
                C_YLLW, C_NRML, C_GREN, local_tps.n_tcp_connect,        C_NRML, C_RED, (local_tps.n_tcp_connect - old_tps.n_tcp_connect), C_NRML,
                C_YLLW, C_NRML, C_GREN, local_tps.n_tcp_close,          C_NRML, C_RED, (local_tps.n_tcp_close - old_tps.n_tcp_close), C_NRML,
                C_YLLW, C_NRML, C_GREN, local_tps.n_smtp_message_send,  C_NRML, C_RED, (local_tps.n_smtp_message_send - old_tps.n_smtp_message_send), C_NRML,
                C_YLLW, C_NRML, C_GREN, local_tps.n_smtp_message_recv,  C_NRML, C_RED, (local_tps.n_smtp_message_recv - old_tps.n_smtp_message_recv), C_NRML);
        old_tps = local_tps;
    }
 }


int smtpStartTPSThreads() {
    int nErr;
    pthread_t clientTh;
    pthread_attr_t clientThAttr;
    pthread_attr_init(&clientThAttr);

    nErr = pthread_attr_setstacksize(&clientThAttr, (10 * 1024 * 1024));

    if ((nErr = pthread_create(&clientTh, &clientThAttr, SMTP_TPS_TH, NULL)) < 0) {
        LOG (LOG_MAJ, "Err. TPS Thread Create Failed. Err.= '%s'\n", strerror(nErr));
        return -1;
    }

    return 0;
}


