#include "transc.h"

// -----------------------------------------

/**
 * @fn void transc_clear_data( transc_t *transc)
 * @brief transc_t 구조체 변수의 data 의 메모리와 전달받을 데이터의 바이트를 0으로 리셋하는 함수
 * @return void
 * @param transc 해당 객체의 멤버 변수들의 값을 리셋하기 위한 transc_t 구조체 변수
 */
void transc_clear_data( transc_t *transc){
	transc->is_recv_header = 0;
	transc->is_recv_body = 0;
	transc->is_send_header = 0;
	transc->is_send_body = 0;
	transc->length = 0;
	transc->recv_bytes = 0;
	transc->send_bytes = 0;
	memset( transc->read_hdr_buf, '\0', MSG_HEADER_LEN);
	memset( transc->write_hdr_buf, '\0', MSG_HEADER_LEN);
	transc->data = NULL;
}

/**
 * @fn uint32_t transc_get_msg_length( transc_t *transc)
 * @brief 전달받은 메시지를 일부만 decode 해서 메시지의 총 길이(Header + Body)를 구하는 함수
 * @return 메시지 길이
 * @param transc 메시지의 길이를 구하기 위한 transc_t 구조체 변수
 */
uint64_t transc_get_msg_length( transc_t *transc){
	if( ( strlen( transc->read_hdr_buf) == 0)){
		return -1;
	}

	// 1 byte = 8 bits
	// len size = 3 bytes
	uint8_t *data = ( uint8_t*)( transc->read_hdr_buf);

	// Big endian
	uint32_t msg_len_b = ( ( ( int)( data[ 1])) << 16) + ( ( ( int)( data[ 2])) << 8) + data[ 3];

	// Little endian
	//uint32_t msg_len_l = ( ( ( int)( data[ 3])) << 16) + ( ( ( int)( data[ 2])) << 8) + data[ 1];

	//printf("msg_len_b : %d\n", msg_len_b);
	//printf("msg_len_l : %d\n", msg_len_l); 

	//int i;
	//for( i = 0; i < 20; i++){
	//	printf("%d = %d\n", i, ( uint8_t)data[i]); 
	//}	

	return msg_len_b;
}

