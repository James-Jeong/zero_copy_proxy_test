#include "pipe.h"

int pipe_open( pipe_t *_pipe){
	if( pipe(_pipe->fd) < 0){
		printf("	| ! Proxy : Fail to open pipe\n");
		return -1;
	}
	return 0;
}

void pipe_close( pipe_t *_pipe){
	close( _pipe->fd[0]);
	close( _pipe->fd[1]);
}

