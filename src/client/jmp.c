#include "jmp.h"

/**
 * @fn jmp_t* jmp_init()
 * @brief 프로토콜 메시지 객체를 생성하고 초기화하기 위한 함수
 * @return 프로토콜 메시지 객체
 */
jmp_t* jmp_init(){
	jmp_t* msg = ( jmp_t*)malloc(sizeof( jmp_t));
	memset( msg->data, 0, sizeof( msg->data));
	return msg;
}

/**
 * @fn void jmp_destroy( jmp_t *msg)
 * @brief 프로토콜 메시지 객체를 삭제하기 위한 함수
 * @return void
 * @param msg 삭제할 프로토콜 객체
 */
void jmp_destroy( jmp_t* msg){
	if( msg->data){
		free( msg->data);
	}
	free( msg);
}

/**
 * @fn int jmp_get_data_len( jmp_t* msg)
 * @brief 프로토콜 메시지 바디의 데이터 길이를 구하는 함수
 * @return 메시지 바디의 데이터 길이
 * @param msg 소유한 데이터 길이를 알고자하는 메시지 객체
 */
int jmp_get_msg_length( jmp_t* msg){
	return msg->hdr.length;
}

/**
 * @fn char* jmp_get_data( jmp_t* msg)
 * @brief 프로토콜 메시지 바디의 데이터를 구하는 함수
 * @return 메시지 바디의 데이터(문자열)
 * @param msg 소유한 데이터를 알고자하는 메시지 객체
 */
char* jmp_get_data( jmp_t* msg){
	return msg->data;
}

/**
 * @fn int jmp_set_msg( jmp_t *msg, uint32_t seq_id, char *data)
 * @brief 프로토콜 메시지의 헤더와 바디를 지정하는 함수
 * @return 성공 여부(1:success, -1:fail)
 * @param msg 설정하려고 하는 메시지 객체
 * @param version 설정하려고 하는 메시지의 version
 * @param data 설정하려고 하는 메시지 데이터
 * @param code 설정하려고 하는 메시지 명령 코드
 */
int jmp_set_msg( jmp_t *msg, uint8_t version, char *data, uint32_t code){
	if( data != NULL){
		int len = strlen( data);

		msg->hdr.version = version;
		msg->hdr.length = len + sizeof( msg->hdr);
		msg->hdr.flag = 0;
		msg->hdr.code = code;
		msg->hdr.app_id = 0;
		msg->hdr.hop_id = 0;
		msg->hdr.end_id = 0;

		memset( msg->data, 0, DATA_MAX_LEN);
		data[ len] = '\0';
		memcpy( msg->data, data, len);
		return 1;
	}
	return -1;
}


/**
 * @fn void jmp_print_msg( jmp_t *msg)
 * @brief 프로토콜 메시지 전체를 출력하는 함수
 * @return void
 * @param msg 출력하려는 메시지의 객체
 */
void jmp_print_msg( jmp_t *msg){
	if( msg == NULL){
		printf("msg null!\n");
		return ;
	}

	printf("----- msg header -----\n");
	printf("| msg version : %d\n", msg->hdr.version);
	printf("| msg length : %d\n", msg->hdr.length);
	printf("| msg flag : %d\n", msg->hdr.flag);
	printf("| msg code : %d\n", msg->hdr.code);
	printf("| msg app_id : %d\n", msg->hdr.app_id);
	printf("| msg hop_id : %d\n", msg->hdr.hop_id);
	printf("| msg end_id : %d\n", msg->hdr.end_id);

	printf("----- msg body -----\n");
	printf("| msg data : %s\n\n", msg->data);
}

