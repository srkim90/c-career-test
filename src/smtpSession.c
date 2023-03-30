#include "main.h"

#define BUCKET_SIZE 4096
smtp_session_t* g_smtp_sessions[BUCKET_SIZE] = {NULL,};
pthread_mutex_t g_session_lock = PTHREAD_MUTEX_INITIALIZER;

int hash_func(const char *string, size_t len) {
    int i;
    int hash;

    hash = 0;
    for (i = 0; i < len; ++i) {
        hash = 65599 * hash + string[i];
    }

    return hash ^ (hash >> 16);
}

void delSmtpSession(char *session_id) {
    int hash_id = hash_func(session_id, strlen(session_id));
    int start_index = hash_id % BUCKET_SIZE;
    int i = start_index;
    pthread_mutex_lock ( &g_session_lock ) ;
    do {
        if(g_smtp_sessions[i] != NULL) {
            smtp_session_t * session =  g_smtp_sessions[i];
            if (strcmp(session->session_id, session_id) == 0) {
                close(session->sock_fd);
                free(session);
                g_smtp_sessions[i] = NULL;
                pthread_mutex_unlock ( &g_session_lock ) ;
                inc_smtp_tps_tcp_close();
                return;
            }
        }
        i = (i + 1) % BUCKET_SIZE;
    } while (i != start_index);
    pthread_mutex_unlock ( &g_session_lock ) ;
}

smtp_session_t *getSmtpSession(char *session_id) {
    int hash_id = hash_func(session_id, strlen(session_id));
    int start_index = hash_id % BUCKET_SIZE;
    int i = start_index;
    pthread_mutex_lock ( &g_session_lock ) ;
    do {
        if(g_smtp_sessions[i] != NULL) {
            if (strcmp(g_smtp_sessions[i]->session_id, session_id) == 0) {
                pthread_mutex_unlock(&g_session_lock);
                return  g_smtp_sessions[i];
            }
        }
        i = (i + 1) % BUCKET_SIZE;
    } while (i != start_index);

    pthread_mutex_unlock ( &g_session_lock ) ;
    return NULL;
}

smtp_session_t *addSmtpSession(smtp_session_t *session) {
    int i;
    int start_index;
    int hash_id;

    hash_id = hash_func(session->session_id, strlen(session->session_id));
    start_index = hash_id % BUCKET_SIZE;
    i = start_index;

    pthread_mutex_lock ( &g_session_lock ) ;
    do {
        if (g_smtp_sessions[i] == NULL) {
            g_smtp_sessions[i] = session;
            pthread_mutex_unlock ( &g_session_lock ) ;
            return session;
        }
        i = (i + 1) % BUCKET_SIZE;
    } while (i != start_index);
    pthread_mutex_unlock ( &g_session_lock ) ;
    LOG(LOG_MAJ, "Error. fail to add smtp session : session store is full");
    free(session);
    return NULL;
};
