#include "client.h"

/**
 * @fn int main( int argc, char **argv)
 * @brief client 구동을 위한 main 함수
 * @return int
 * @param argc 매개변수 개수
 * @param argv ip 와 포트 정보
 */
int main( int argc, char **argv){
	if( argc != 2){
		printf("	| ! need param : hostname\n");
		return -1;
	}

	client_t *client = client_init( argv[1]);
    if( client == NULL){
        printf("    | ! Client : Fail to initialize\n");
        return -1;
    }

	client_process_data( client);

	client_destroy( client);
}



// -----------------------------------------

/**
 * @fn client_t* client_init()
 * @brief server 객체를 생성하고 초기화하는 함수
 * @return 생성된 server 객체
 */
client_t* client_init( char* hostname){
	client_t *client = ( client_t*)( malloc( sizeof( client_t)));

	if( client == NULL){
		printf("        | ! Client : Fail to allocate memory\n");
		return NULL;
	}

	struct hostent* he;
	if( ( he = gethostbyname( hostname)) == NULL){
		printf("        | ! Client : Fail to get hostent from hostname\n");
		free( client);
		return NULL;
	}

	if( ( client->fd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1){
		printf("        | ! Client : Fail to open socket\n");
		free( client);
		return NULL;
	}

	memset( &client->server_addr, 0, sizeof( client->server_addr));
	client->server_addr.sin_family = AF_INET;
	client->server_addr.sin_addr = *( ( struct in_addr*)( he->h_addr));
	client->server_addr.sin_port = htons(SERVER_PORT);
	bzero( &( client->server_addr.sin_zero), 8);

	if( connect( client->fd, ( struct sockaddr*)( &client->server_addr), sizeof( struct sockaddr))){
		printf("        | ! Client : Fail to connect with Server\n");
		close( client->fd);
		free( client);
		return NULL;
	}

	printf("        | @ Client : Success to create a object & connect with Server\n");
	printf("        | @ Client : Welcome\n\n");
	return client;
}

/**
 * @fn void client_destroy( client_t *client)
 * @brief client 객체를 삭제하기 위한 함수
 * @return void
 * @param client 삭제하기 위한 client 객체
 */
void client_destroy( client_t *client){
	close( client->fd);
	free( client);
	printf("        | @ Client : Success to destroy the object\n");
	printf("        | @ Client : BYE\n\n");
}

/**
 * @fn void client_process_data( client_t *client)
 * @brief server 로 요청을 보내서 응답을 받는 함수
 * @return void
 * @param client 요청을 하기 위한 client 객체
 */
void client_process_data( client_t *client){
	int server_addr_size = 0;
	char read_buf[ BUF_MAX_LEN];
	char send_buf[ BUF_MAX_LEN];
	ssize_t recv_bytes, send_bytes;

	printf("        | @ Client : put the data\n");
	while(1){
		char msg[ BUF_MAX_LEN];
		printf("        | @ Client : > ");
		fgets( msg, BUF_MAX_LEN, stdin);
		msg[ strlen( msg) - 1] = '\0';
		snprintf( send_buf, BUF_MAX_LEN, "%s", msg);
	
		jmp_t send_msg;
		jmp_set_msg( &send_msg, 1, send_buf, 1);
		jmp_print_msg( &send_msg);
		if( ( send_bytes = write( client->fd, &send_msg, jmp_get_msg_length( &send_msg))) <= 0){
			printf("        | ! Client : Fail to send msg (bytes:%d) (errno:%d)\n", send_bytes, errno);
		}
		else{
			printf("        | @ Client : Success to send msg to Server (bytes:%d)\n", send_bytes);

			if( !memcmp( send_buf, "q", 1)){
				printf("        | @ Client : Finish\n");
				break;
			}

			jmp_t recv_msg;
			if( ( recv_bytes = read( client->fd, &recv_msg, BUF_MAX_LEN)) <= 0){
				printf("        | ! Client : Fail to recv msg (bytes:%d) (errno:%d)\n\n", recv_bytes, errno);
				break;
			}
			else{
				jmp_print_msg( &recv_msg);
				snprintf( read_buf, BUF_MAX_LEN, "%s", jmp_get_data( &recv_msg)); 
				read_buf[ recv_bytes] = '\0';
				printf("        | @ Client : < %s (%lu bytes)\n", read_buf, recv_bytes);
			}
		}
		printf("\n");
	}
}

