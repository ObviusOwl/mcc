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

struct thread_arg_data{
	int *pipe;
	int *socket;
};

void *send_thread_main( void *arg ){
	struct thread_arg_data *arg_data;
	arg_data = (struct thread_arg_data* ) arg;
	char readbuffer[1024];
	ssize_t count = 0;
	ssize_t count2 = 0;
	
	while( 1 ){
		count = recv( *(arg_data->socket), readbuffer, sizeof(readbuffer), 0 );
		if( count == 0 ){
			break;
		}else if( count < 0 ){
			perror("failed to read from socket");
		}else{
			count2 = write( *(arg_data->pipe), readbuffer, count );
			if( count2 < 0 ){
				perror("failed to write to pipe");
			}
		}
	}
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
		pthread_t send_thread;
		char readbuffer[1024];
		int listen_sock, client_sock;
		struct sockaddr_un sock_addr;
		struct thread_arg_data thread_data;
		
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
		r = listen(listen_sock, 5);
		if( r == -1 ){
			perror("faild to listen on socket");
			exit(EXIT_FAILURE);
		}
		thread_data.pipe = &(pipe2[1]);
		
		while(1){
			client_sock = accept( listen_sock, NULL, NULL );
			thread_data.socket = &client_sock;
			pthread_create( &send_thread, NULL, send_thread_main, (void*) &thread_data);
			if( client_sock == -1 ){
				perror("faild accepting connection");
				break;
			}
			int count = 0;
		    while( 1 ){
				count = read(pipe1[0], readbuffer, sizeof(readbuffer) );
				if( count > 0 ){
					if( send( client_sock, readbuffer, count, MSG_NOSIGNAL ) < 0 ){
						perror("faild to send data");
						break;
					}
				}else{
					break;
				}
    		}
			pthread_join( send_thread, NULL);
			thread_data.socket = NULL;
    		close(client_sock);
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
