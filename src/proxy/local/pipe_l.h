#pragma once
#ifndef __PIPE_H__
#define __PIPE_H__

#include <stdio.h>

/// @struct pipe_t
/// @brief proxy 에서 thread 별 splice pipe 관리를 위한 구조체
typedef struct pipe_s pipe_t;
struct pipe_s{
	/// pipe file descriptor
	int fd[2];
	/// 전달 받은 pipe 데이터 크기
	int bytes;
};

int pipe_open( pipe_t *_pipe);
void pipe_close( pipe_t *_pipe);

#endif

