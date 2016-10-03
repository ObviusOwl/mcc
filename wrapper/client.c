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

	while( 1 ){
		char *line = NULL;
		size_t size;
		if( getline(&line, &size, stdin) == -1) {
			perror("faild to read line from stdin");
			break;
		}
		if( send( *sock, line, size*sizeof(char), MSG_NOSIGNAL ) < 0 ){
			perror("faild to send data");
			break;
		}
		if( strncmp( line, "bye", 3*sizeof(char) ) == 0 ){
			break;
		}
	}
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
		if( count == 0 ){
			break;
		}
		if( strncmp( readbuffer, "msg", 3*sizeof(char) ) == 0 ){
			write( STDOUT_FILENO, &(readbuffer[3]), count-3*sizeof(char) );
		}else{
			write( STDOUT_FILENO, readbuffer, count );
		}
    }
	pthread_join( input_thread, NULL);
	close(sock);
}
