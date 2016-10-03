#define _XOPEN_SOURCE  700

#include "args.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <pthread.h>

#define LISTEN_THREAD_COUNT 4

typedef struct _sender_thread{
	int pipe;
	int socket;
	pthread_t *thread;
	pthread_mutex_t lock;
	int willclose;
} SenderThread;

void *thread_main( void *arg ){
	SenderThread *self;
	self = (SenderThread* ) arg;
	char readbuffer[1024];
	ssize_t count = 0;
	ssize_t count2 = 0;
	ssize_t prefix_len = 3*sizeof(char);
	
	while( 1 ){
		pthread_mutex_lock( &(self->lock) );
		if( self->willclose == 1 ){
			pthread_mutex_unlock( &(self->lock) );
			break;
		}
		pthread_mutex_unlock( &(self->lock) );
		
		count = recv( self->socket, readbuffer, sizeof(readbuffer), 0 );
		if( count == 0 ){
			//break;
		}else if( count < 0 ){
			perror("failed to read from socket");
		}else{
			if( count > prefix_len ){
				// message to stdin of child process
				if( strncmp( readbuffer, "msg", prefix_len) == 0 ){
					count2 = write( self->pipe, &(readbuffer[3]), count );
					if( count2 < 0 ){
						perror("failed to write to pipe");
					}
				// client disconnect
				}else if( strncmp( readbuffer, "bye", prefix_len) == 0 ){
					pthread_mutex_lock( &(self->lock) );
					self->willclose = 1;
					pthread_mutex_unlock( &(self->lock) );
				}else{
					fprintf(stderr,"unknown command %.3s\n", readbuffer);
				}
			}
		}
	}
	pthread_mutex_lock( &(self->lock) );
	free( self->thread );
	self->thread = NULL;
	self->willclose = 0;
	close(self->socket );
	pthread_mutex_unlock( &(self->lock) );
	pthread_exit(NULL);
}
void *send_thread_main( void *arg ){
	SenderThread *threads;
	threads = (SenderThread*) arg;
	SenderThread* self = &threads[0];
	char readbuffer[1024];
	ssize_t count = 0;
	int i = 0;
	int ok= 0;
	ssize_t prefix_len = 3*sizeof(char);

	while( 1 ){
		count = read(self->pipe, &(readbuffer[3]), sizeof(readbuffer)-prefix_len );
		if( count > 0 ){
			memcpy( readbuffer, "msg", prefix_len );
			for( i=1; i<LISTEN_THREAD_COUNT+1; i++ ){
				ok = 0;
				pthread_mutex_lock( &(threads[i].lock) );
				if( threads[i].thread != NULL ){
					ok = 1;
				}
				pthread_mutex_unlock( &(threads[i].lock) );
				if( ok == 1 ){
					if( send( (&threads[i])->socket, readbuffer, count+prefix_len, MSG_NOSIGNAL ) < 0 ){
						perror("faild to send data");
						// shutdown client thread
						pthread_mutex_lock( &(threads[i].lock) );
						threads[i].willclose = 1;
						pthread_mutex_unlock( &(threads[i].lock) );
						continue;
					}
				}
			}
		}else{
			break;
		}
	}
	pthread_mutex_lock( &(self->lock) );
	free( self->thread );
	self->thread = NULL;
	self->willclose = 0;
	pthread_mutex_unlock( &(self->lock) );
	pthread_exit(NULL);
}

int main(int argc, char **argv){
	struct arguments config;
	config.basedir = NULL;
	config.socket_file = NULL;
	config.command = NULL;
	argp_parse(&argp, argc, argv, 0, 0, &config);

	int r, r2;
	int pipe1[2];
	int pipe2[2];
	r = pipe( pipe1 );
	r2 = pipe( pipe2 );
	if( r == -1 || r2 == -1 ){
		perror("faild to create pipe");
		exit(EXIT_FAILURE);
	}

	pid_t pid = fork();
	if( pid > 0 ){
		// parent
		close( pipe1[1] );
		close( pipe2[0] );
		SenderThread threads[LISTEN_THREAD_COUNT + 1];

		char readbuffer[1024];
		int i;
		int listen_sock, client_sock;
		struct sockaddr_un sock_addr;
		sock_addr.sun_family = AF_UNIX;
		strcpy(sock_addr.sun_path, config.socket_file );

		listen_sock = socket(AF_UNIX, SOCK_STREAM, 0);
		if(listen_sock == -1 ){
			perror("faild to create socket");
			exit(EXIT_FAILURE);
		}
		unlink(sock_addr.sun_path);

		r = bind(listen_sock, (struct sockaddr *) &sock_addr, 
			strlen(sock_addr.sun_path)+sizeof(sock_addr.sun_family) );
		if( r == -1 ){
			perror("faild to bind to socket");
			exit(EXIT_FAILURE);
		}
		// do not accept more clients than we have threads (4)
		r = listen(listen_sock, LISTEN_THREAD_COUNT);
		if( r == -1 ){
			perror("faild to listen on socket");
			exit(EXIT_FAILURE);
		}

		// init threads, threads[0] is the only sender thread
		for( i=0; i< LISTEN_THREAD_COUNT+1 ; i++ ){
			threads[i].thread = NULL;
			threads[i].willclose = 0;
			pthread_mutex_init( &(threads[i].lock), NULL );
		}
		for( i=1; i< LISTEN_THREAD_COUNT+1 ; i++ ){
			threads[i].pipe = pipe2[1];
		}
		threads[0].pipe = pipe1[0];
		threads[0].thread = (pthread_t *) malloc( sizeof(pthread_t) );
		pthread_create( threads[0].thread, NULL, send_thread_main, (void*) &threads);
		
		while(1){
			// accept new connection
			client_sock = accept( listen_sock, NULL, NULL );
			if( client_sock == -1 ){
				perror("faild accepting connection");
				continue;
			}
			// get a free thread 
			SenderThread *curr_thread = NULL;
			for( i=1; i<LISTEN_THREAD_COUNT+1; i++ ){
				pthread_mutex_lock( &(threads[i].lock) );
				if( (&threads[i])->thread == NULL ){
					curr_thread = &(threads[i]);
					break;
				}else{
					pthread_mutex_unlock( &(threads[i].lock) );
				}
			}
			if( curr_thread == NULL ){
				continue;
			}
			curr_thread->socket = client_sock;
			curr_thread->thread = (pthread_t *) malloc( sizeof(pthread_t) );
			pthread_mutex_unlock( &(curr_thread->lock) );
			
			pthread_create( curr_thread->thread, NULL, thread_main, (void*) curr_thread);
		}
		for( i=0; i<LISTEN_THREAD_COUNT+1; i++ ){
			if( threads[i].thread != NULL ){
				pthread_join( *((&(threads[i]))->thread), NULL);
			}
			pthread_mutex_destroy( &(threads[i].lock) );
		}
		close(listen_sock);
		unlink(sock_addr.sun_path);
		int stat, err;
		err = waitpid( pid, &stat, 0 );
		if( WIFEXITED(stat) ){
			return WEXITSTATUS(stat);
		}
		if( err < 0 ){
			return 127;
		}
	}else if( pid == 0 ){
		// child
		int r = 0;
		close( pipe1[0] );
		close( pipe2[1] );
		dup2( pipe1[1] , STDOUT_FILENO );
		dup2( pipe1[1] , STDERR_FILENO );
		dup2( pipe2[0] , STDIN_FILENO );

		if( config.basedir != NULL ){
			r = chdir( config.basedir );
			if( r == -1 ){
				perror( "failed to change working directory" );
			}
		}
		char *opts[] = { NULL };
		execvp( config.command , opts );
	}else{
		// fork failed
		return 127;
	}
	return 0;

}
