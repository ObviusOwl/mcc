#define _XOPEN_SOURCE  700

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>

//#include <pthread.h>


int main(int argc, char **argv){
	if( argc < 2 ){
		printf( "first parameter must be the path to the socket" );
	}
	unsigned int sock;
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

	ssize_t count = 0;
	while( 1 ) {
		count = recv(sock, readbuffer, sizeof(readbuffer), 0);
		if( count == 0 ){
			break;
		}
		write( STDOUT_FILENO, readbuffer, count );
    }
	
	close(sock);
}
