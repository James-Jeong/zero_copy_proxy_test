#pragma once

#ifndef __JMP_H__
#define __JMP_H__

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#define DATA_MAX_LEN 1024


typedef unsigned short ushort;

/// @struct jmp_hdr_t
/// @brief 통신을 위한 프로토콜 헤더 구조체, 총 7 바이트
typedef struct jmp_hdr_s jmp_hdr_t;
struct jmp_hdr_s{
	/// message protocol version
	uint8_t version;
	/// message header + body length
	uint32_t length:24;
	/// message flag
	uint8_t flag; /**< message flag */
	/// message command code
	uint32_t code:24;
	/// message application id
	uint32_t app_id;
	/// message hop-by-hop identifier
	uint32_t hop_id;
	/// message end-to-end identifier
	uint32_t end_id;
};

/// @struct jmp_t
/// @brief 통신을 위한 프로토콜 구조체
typedef struct jmp_s jmp_t;
struct jmp_s{
	/// message header
	jmp_hdr_t hdr;
	/// message data
	char data[ DATA_MAX_LEN];
};

jmp_t* jmp_init();
void jmp_destroy( jmp_t *msg);
int jmp_get_msg_length( jmp_t *msg);
char* jmp_get_data( jmp_t *msg);
int jmp_set_msg( jmp_t *msg, uint8_t version, char *data, uint32_t code);
void jmp_print_msg( jmp_t *msg);

#endif

