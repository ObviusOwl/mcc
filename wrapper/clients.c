#include "config.h"
#include "clients.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>

char* do_command( SenderThread* self, char* magic_start ){
	if( strncmp( magic_start, "bye", 3 ) == 0 ){
		pthread_mutex_lock( &(self->lock) );
		self->willclose = 1;
		pthread_mutex_unlock( &(self->lock) );
		return magic_start+3;
	}else if( strncmp( magic_start, "test", 4 ) == 0 ){
		fprintf(stderr,"test command\n");
		return magic_start+4;
	}else{
		fprintf(stderr,"unknown command\n");
	}
	return magic_start;
}

void *thread_main( void *arg ){
	SenderThread *self;
	self = (SenderThread* ) arg;
	char* magic = "::mc::"; 
	// data read from recv
	char readbuffer[1024];
	// data to be send to pipe
	char sendbuffer[1024];
	// position of first magic char from current command spec
	char* command_curr_pos = NULL;
	// position of first non magic char from current command spec
	char* command_curr_pos2 = NULL;
	// position of first non magic char from last command spec
	char* command_last_pos = NULL;
	// current write position in send buffer
	char* send_cur_pos = NULL;
	// byte send counts
	ssize_t count = 0;
	ssize_t count2 = 0;
	
	while( 1 ){
		// check if thread termination is pending
		pthread_mutex_lock( &(self->lock) );
		if( self->willclose == 1 ){
			pthread_mutex_unlock( &(self->lock) );
			break;
		}
		pthread_mutex_unlock( &(self->lock) );
		
		// block reading
		count = recv( self->socket, readbuffer, sizeof(readbuffer), 0 );
		if( count == 0 ){
			continue;
		}else if( count < 0 ){
			perror("failed to read from socket");
		}else{
			// parse for magic strings describing commands
			command_last_pos = readbuffer;
			send_cur_pos = sendbuffer;
			command_curr_pos = strstr( readbuffer, magic );
			while( command_curr_pos != NULL ){
				// run command & get position after command spec
				command_curr_pos2 = do_command( self, command_curr_pos+6 );
				// copy data from before command spec
				strncpy( send_cur_pos, command_last_pos, command_curr_pos-command_last_pos);
				// update write position in sendbuffer
				send_cur_pos = send_cur_pos + (command_curr_pos-command_last_pos);
				// next data copy starts at end of command spec
				command_last_pos = command_curr_pos2;
				// search for next command spec
				command_curr_pos = strstr( command_curr_pos2, magic );
			}
			// copy the rest
			strcpy( send_cur_pos, command_last_pos );
			// todo: handle partial magic string

			// finally write to child process stdin
			count2 = write( self->pipe, sendbuffer, count );
			if( count2 < 0 ){
				perror("failed to write to pipe");
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

	while( 1 ){
		count = read(self->pipe, readbuffer, sizeof(readbuffer) );
		if( count > 0 ){
			for( i=1; i<LISTEN_THREAD_COUNT+1; i++ ){
				ok = 0;
				pthread_mutex_lock( &(threads[i].lock) );
				if( threads[i].thread != NULL ){
					ok = 1;
				}
				pthread_mutex_unlock( &(threads[i].lock) );
				if( ok == 1 ){
					if( send( (&threads[i])->socket, readbuffer, count, MSG_NOSIGNAL ) < 0 ){
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
