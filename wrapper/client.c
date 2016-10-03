#define _XOPEN_SOURCE  700

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <pthread.h>


void *thread_main( void *arg ){
	unsigned int *sock;
	sock = (unsigned int *) arg;
	char *line = (char *) malloc( 20*sizeof(char) );
	size_t size = 20*sizeof(char);
	int do_quit = 0;

	while( 1 ){
		if( getline(&line, &size, stdin) == -1) {
			perror("faild to read line from stdin");
			break;
		}
		if( strncmp( line, ":", 1 ) == 0 ){
			if( strlen(line)+5 > size ){
				line = realloc( line, size+6 );
			}
			memmove( line+5, line, strlen(line)+1 );
			memcpy( line, "::mc::", 5);
			do_quit = 1;
		}
		if( send( *sock, line, size, MSG_NOSIGNAL ) < 0 ){
			perror("faild to send data");
			break;
		}
		if( do_quit == 1 ){
			break;
		}
	}
	free( line );
	pthread_exit(NULL);
}


int main(int argc, char **argv){
	if( argc < 2 ){
		printf( "first parameter must be the path to the socket" );
	}
	unsigned int sock;
	pthread_t input_thread;
	char readbuffer[10];
	struct sockaddr_un socket_addr;
	socket_addr.sun_family = AF_UNIX;
	strcpy(socket_addr.sun_path, argv[1] );
	int r = 0;
		
	sock = socket(AF_UNIX, SOCK_STREAM, 0);
	r = connect(sock, (struct sockaddr *) &socket_addr, 
		strlen(socket_addr.sun_path) + sizeof(socket_addr.sun_family) );
	if( r == -1) {
        perror("failed to connect to socket");
		exit(EXIT_FAILURE);
    }
	pthread_create( &input_thread, NULL, thread_main, (void*) &sock);

	ssize_t count = 0;
	while( 1 ) {
		count = recv(sock, readbuffer, sizeof(readbuffer), 0);
		if( count <= 0 ){
			break;
		}
		write( STDOUT_FILENO, readbuffer, count );
    }
	pthread_join( input_thread, NULL);
	close(sock);
}
