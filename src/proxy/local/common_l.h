#pragma once
#ifndef __COM_HDR_H__
#define __COM_HDR_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

/// @enum ERROR
/// @brief 각종 프로그램 에러 열거형
enum ERROR{
	RECV_COMPLETE	= 3,
	NOT_RECV		= 2,
	ERRNO_EAGAIN	= 1,
	NORMAL			= 0,
	FD_ERR			= -1,
	OBJECT_ERR		= -1,
	SOC_ERR			= -2,
	NOT_EXIST		= -2,
	ZERO_BYTE		= -3,
	NEGATIVE_BYTE	= -4,
	BUF_ERR			= -5,
	HOST_ERR		= -6,
	PTHREAD_ERR		= -7,
	INTERRUPT		= -8,
	UNKNOWN			= -9
};

#endif

