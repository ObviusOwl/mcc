#include "config.h"
#include "args.h"
#include "clients.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>

#include <pthread.h>
#include <sys/select.h>

int main_wakeup_pipe;
int main_want_quit = 0;

static void sighandler_request_sutdown( int signum ){
	int c = 0;
	main_want_quit = 1;
	c = write( main_wakeup_pipe, &main_want_quit, sizeof(int) );
	if( c < 0 ){
		
	}
}

int main(int argc, char **argv){
	struct arguments config;
	config.basedir = NULL;
	config.socket_file = NULL;
	config.command = NULL;
	argp_parse(&argp, argc, argv, 0, 0, &config);

	int r, r2, r3;
	int pipe1[2];
	int pipe2[2];
	int pipe3[2]; // main wakeup pipe
	int yes = 1;
	r = pipe( pipe1 );
	r2 = pipe( pipe2 );
	r3 = pipe( pipe3 );
	if( r == -1 || r2 == -1 || r3 == -1 ){
		perror("faild to create pipe");
		exit(EXIT_FAILURE);
	}
	main_wakeup_pipe = pipe3[1];
		
	pid_t pid = fork();
	if( pid > 0 ){
		// parent
		close( pipe1[1] );
		close( pipe2[0] );

		struct sigaction sigchld_action;
		sigchld_action.sa_handler = sighandler_request_sutdown;
		sigchld_action.sa_flags = SA_NOCLDSTOP;
		r = sigaction(SIGCHLD, &sigchld_action, NULL);
		if( r == -1 ){
			perror("faild to initialize signal handler");
			exit(EXIT_FAILURE);		
		}

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
			r = pipe( threads[i].wakeup_pipe );
			if( r == -1 ){
				perror("faild to create pipe");
				exit(EXIT_FAILURE);
			}
		}
		for( i=1; i< LISTEN_THREAD_COUNT+1 ; i++ ){
			threads[i].pipe = pipe2[1];
		}
		threads[0].pipe = pipe1[0];
		threads[0].thread = (pthread_t *) malloc( sizeof(pthread_t) );
		pthread_create( threads[0].thread, NULL, send_thread_main, (void*) &threads);

		struct pollfd poll_listen_sock_fd[] = {
			{listen_sock, POLLIN, 0 },
			{pipe3[0], POLLIN, 0 }
		};
		
		while(1){
			// quit if requested
			if( main_want_quit ){
				break;
			}
			// accept new connection
			poll( poll_listen_sock_fd, 2, -1);
			// wakeup
			if( poll_listen_sock_fd[1].revents & POLLIN ){
				continue;
			}
			// only interrested in accepts
			if( ! (poll_listen_sock_fd[0].revents & POLLIN) ){
				continue;
			}
			client_sock = accept4( listen_sock, NULL, NULL, SOCK_NONBLOCK );
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
		
		// cleanup and exit
		// join threads and free allocated mem
		for( i=0; i<LISTEN_THREAD_COUNT+1; i++ ){
			pthread_mutex_lock( &(threads[i].lock) );
			threads[i].willclose = 1;
			pthread_mutex_unlock( &(threads[i].lock) );
			if( threads[i].thread != NULL ){
				// thread is active
				r = write( threads[i].wakeup_pipe[1], &yes, sizeof(int) );
				if( r < 0 ){
					perror( "failed to write to wakeup pipe" );
				}
				pthread_join( *((&(threads[i]))->thread), NULL);
			}
			pthread_mutex_destroy( &(threads[i].lock) );
			close( threads[i].wakeup_pipe[0] );
			close( threads[i].wakeup_pipe[1] );
		}
		close(listen_sock);
		unlink(sock_addr.sun_path);
		int stat, err;
		err = waitpid( pid, &stat, 0 );
		if( WIFEXITED(stat) ){
			exit( WEXITSTATUS(stat) );
		}
		if( err < 0 ){
			perror( "waitpid" );
			exit( EXIT_FAILURE );
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
		printf("fork failed\n");
		exit( EXIT_FAILURE );
	}
	exit( EXIT_SUCCESS );
}
