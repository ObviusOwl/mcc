#ifndef CLIENTS_H
#define CLIENTS_H

#include <pthread.h>

typedef struct _sender_thread{
	int pipe;
	int socket;
	pthread_t *thread;
	pthread_mutex_t lock;
	int willclose;
	int wakeup_pipe[2];
} SenderThread;


char* do_command( SenderThread* self, char* magic_start );
void *thread_main( void *arg );
void *send_thread_main( void *arg );

#endif //CLIENTS_H