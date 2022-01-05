#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include<errno.h>
#define PORT 1088

struct message_attribute{
    char protocol[6];
    char type;     
    char status;       
    int length;     
    char payload[1024]; 
} __attribute__ ((packed));
struct message_attribute message, send_message;
void send_quit_request(int newSocket, int *connected){
    struct message_attribute message, recv_message;
    char correct_type = 0xAC;

    if( *connected )
    {
        printf("Disconnecting... ");
        message.protocol[0] = 0xe3;
        strcat(message.protocol, "myftp");
        message.type = 0xAB;
        message.length = 12;
        memset(message.payload, '\0', 1024);
        memset(recv_message.payload, '\0', 1024);
        
        while ( send(newSocket,(char *)&message,message.length, 0) != 12 );
        recv(newSocket,(char*)&recv_message, sizeof(struct message_attribute), 0);

        if( recv_message.type == correct_type )
            printf("Success!\n");
        else
            printf("Error Encountered!\n");
    }
    else
    {
        printf("(No opened connection to terminate)\n");
    }
    *connected = 0;
}
void send_open_conn_request(int newSocket){
    struct message_attribute send_message;
    send_message.protocol[0] = 0xe3;
    strcat(send_message.protocol, "myftp");
    send_message.type = 0xA1;
    send_message.length = 12;
    memset(send_message.payload, '\0', 1024);
    send(newSocket, (char *)&send_message, send_message.length, 0);
}

char server_IP[1024];
int server_port_number;
char user_name[1024];
char user_pswrd[1024];
char filename[1024];

int recvn(int newSocket, void* buf, int buf_len){
    int n_left = buf_len;
    int n = 0;
    while (n_left > 0){
        if ((n = recv(newSocket, buf + (buf_len - n_left), n_left, 0)) < 0){
            if (errno == EINTR)
                n = 0;
            else
                return -1;
        } else if (n == 0){
            return 0;
        }
        n_left -= n;
    }
    return buf_len;
}
int option(char str[]){
    if( strcasecmp(str,"connect") == 0 )   
        return 1;
    if( strcasecmp(str,"user") == 0 )   
        return 2;
    if( strcasecmp(str,"ls") == 0 )     
        return 3;
    if( strcasecmp(str,"get") == 0 )    
        return 4;
    if( strcasecmp(str,"put") == 0 )    
        return 5;
    if( strcasecmp(str,"del") == 0 )    
        return 6;
    return 0;
}
int sendn(int newSocket, const void* buf, int buf_len){
    int n_left = buf_len;
    int n;
    while (n_left > 0){
        if ((n = send(newSocket, buf + (buf_len - n_left), n_left, 0)) < 0){
                if (errno == EINTR)
                        n = 0;
                else
                        return -1;
        } else if (n == 0){
                return 0;
        }
        n_left -= n;
    }
    return buf_len;
}

int upload_file(int newSocket, FILE * fb){
    struct message_attribute data_message;
    int size;

    data_message.protocol[0] = 0xe3;
    strcat(data_message.protocol, "myftp");
    data_message.type = 0xFF;
    data_message.status = 1;

    do{
        size = fread(data_message.payload, 1, 1024, fb);
        if (size < 1024)
            data_message.status = 0;
        data_message.length = 12 + size;

        if (sendn(newSocket, (char *)&data_message,sizeof(struct message_attribute)) == -1){
            printf("%d\n",errno);
            exit(-1);
        }
    } while( data_message.status != 0 );
    return 0;
}
int download_file(int newSocket, FILE * fb){
    struct message_attribute data_message;
    char correct_type = 0xFF;

    do {
        if ( recvn(newSocket, ((char*)&data_message), sizeof(struct message_attribute)) == -1 ){
            printf("Error: Client - Failed to receive file.\n");
            exit(-1);
        }

        if (data_message.type != correct_type){
            printf("Wrong header received, program terminated");
            exit(-1);
        }

        fwrite(data_message.payload, 1, data_message.length-12, fb);

    } while ( data_message.status != 0 );
    return 0;
}
int main(int argc , char *argv[]){

	int clientSocket, ret, newSocket;
	struct sockaddr_in serverAddr;
	char buffer[1024];
	char* token;
	int quit;
	int connected = 0;
	char *t_safe;
	int authenticated = 0;
	char temp_server_IP[1024];
    int temp_server_port_number;
    char temp_user_name[1024];
    char temp_user_pswd[1024];
	FILE *f;
	struct sockaddr_in server;
	clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	char send_type;

    quit=0; 
	while(quit==0){
		printf("Client: \t");
		fgets(buffer, 1024, stdin);
		buffer[strlen(buffer)-1]='\0';
		if (strcasecmp(buffer,"quit") != 0) {

            if( token = strtok_r(buffer," \n", &t_safe), token == 0 ){
                printf("%d\n",errno);
                continue;
            }
            if( connected == 0 && option(token)>1 ){
                printf("%d\n",errno);
                continue;
            }
            if( option(token)>2 && authenticated == 0 ){
                printf("%d\n",errno);
                continue;
            }
	
			if(strcmp(buffer, "QUIT") == 0){
				close(clientSocket);
				//printf("Goodbye!\n");
				send(clientSocket, "Goodbye!\n", strlen("Goodbye!\n"), 0);
				exit(1);
			}
			if(strcmp(buffer, "connect")==0){
				    connected = 0;
                    token = strtok_r(NULL," \n", &t_safe);
                    if( token == NULL ){
                        printf("Usage: connect server_ip port_number\n");
                        continue;
                    }
                    strcpy(temp_server_IP, token);
                    token = strtok_r(NULL," \n", &t_safe);
                    if( token == NULL ){
                        printf("Usage: open SERVER_IP PORT_NUMBER\n");
                        continue;
                    }
                    temp_server_port_number = atoi(token);
                    if( temp_server_port_number <= 0 ){
                        printf("Error: Client - Bad Port Number\n");
                        continue;
                    }
                    token = strtok_r(NULL," \n", &t_safe);
                    if( token != NULL ){
                        printf("Usage: open SERVER_IP PORT_NUMBER\n");
                        continue;
                    }

                    strcpy(server_IP, temp_server_IP);
                    server_port_number = temp_server_port_number;

                    newSocket = socket(AF_INET , SOCK_STREAM , 0);
                    if (newSocket == -1){
                        printf("Error: Client - Could not create socket\n");
                    } else {
                        puts("Welcome to aSa file server.\n");
                    }
 
                    server.sin_addr.s_addr = inet_addr(server_IP);
                    server.sin_family = AF_INET;
                    server.sin_port = htons( server_port_number );
                 
                    if (connect(newSocket , (struct sockaddr *)&server , sizeof(server)) < 0){
                        perror("Error: Client - Connection failed. ");
                        return 1;
                    } else {
                        puts("Welcome to aSa file server\n");
                    }

                    send_open_conn_request(newSocket);
                    printf("OPEN_CONN_REQUEST sent\n");

                    memset(send_message.payload, '\0', 1024);
                    recv(newSocket, (char *)&send_message, sizeof(struct message_attribute), 0);
                    if (send_message.status == '0'){
                        printf("%d.\n",errno);
                    } else {
                        printf("Client - Server connection accepted.\n");
                        connected = 1;
                    }

			}
			if(strcmp(buffer, "USER")==0){
				authenticated = 0;
                    token = strtok_r(NULL," \n", &t_safe);
                    if( token == NULL ){
                        printf("Usage: auth USER_ID USER_PASSWORD\n");
                        continue;
                    }
                    strcpy(temp_user_name,token);
                    token = strtok_r(NULL," \n", &t_safe);
                    if( token == NULL ){
                        printf("Usage: auth USER_ID USER_PASSWORD\n");
                        continue;
                    }
                    strcpy(temp_user_pswd,token);
                    token = strtok_r(NULL," \n",&t_safe);
                    if( token != NULL ){
                        printf("Usage: auth USER_ID USER_PASSWORD\n");
                        continue;
                    }
                    strcpy(user_name, temp_user_name);
                    strcpy(user_pswrd, temp_user_pswd);

                    send_message.protocol[0] = 0xe3;
                    strcat(send_message.protocol, "myftp");
                    send_message.type = 0xA3;
                    send_message.length = 12 + strlen(user_name) + strlen(user_pswrd) + 1;
                    strcpy(send_message.payload, user_name);
                    strcat(send_message.payload, " ");
                    strcat(send_message.payload, user_pswrd);
                    while( send(newSocket, (char*)&send_message, send_message.length, 0) != 12 + strlen(user_name)+strlen(user_pswrd)+1);
                    printf("AUTH_REQUEST sent\n");

                    recv(newSocket, (char *)&send_message,sizeof(struct message_attribute), 0);
                    printf("AUTH_REPLY received\n");
                    send_type = 0xA4;
                    if (send_message.type != send_type){
                        printf("Error: Client - Wrong header received, terminating connection");
                        exit(-1);
                    }
                    if (send_message.status == 1)
                    {
                        printf("200 User test granted to access.\n");
                        authenticated = 1;
                    }
                    else
                    {
                        printf("400 User not found.Please try with another user.\n");
                        close(newSocket);
                    }
			}
			if(strcmp(buffer,"ls")==0){
				 if( token = strtok_r(NULL," \n", &t_safe), token != NULL ){
                            printf("Usage: ls (no argument is needed)\n");
                            continue;
                    }
                    send_message.protocol[0] = 0xe3;
                    strcat(send_message.protocol, "myftp");
                    send_message.type = 0xA5;
                    send_message.length = 12;
                    while ( send(newSocket, (char*)&send_message, send_message.length, 0) != 12 );
                    recv(newSocket, (char *)&send_message,sizeof(struct message_attribute), 0);
                    if( send_type = 0xA6, send_type != send_message.type){
                        printf("Error: Client - Wrong header received, terminating connection");
                        exit(-1);
                    }
                    printf("%s", send_message.payload);

			}
			if(strcmp(buffer, "get")==0){
				token = strtok_r(NULL," \n", &t_safe);
                    if( token == NULL ){
                        printf("Usage: get FILENAME\n");
                        continue;
                    }
                    strcpy(filename, token);
                    token = strtok_r(NULL," \n", &t_safe); 
                    if( token != NULL ){
                        printf("Usage: get FILENAME\n");
                        continue;
                    }
                    printf("Downloading \"%s\"...\n",filename);
                    send_message.protocol[0] = 0xe3;
                    strcat(send_message.protocol, "myftp");
                    send_message.type = 0xA7;
                    strcpy(send_message.payload, filename);
                    send_message.length = 12 + strlen(filename);
                    send(newSocket,(char *)(&send_message),send_message.length, 0);

                    recv(newSocket,(char *)(&send_message),sizeof(struct message_attribute)+1, 0);
                    send_type = 0xA8;
                    if( send_type != send_message.type ){
                        printf("Error: Client - Wrong header received, terminating connection");
                        exit(-1);
                    }
                    if (send_message.status == 1){
                        f = fopen(filename, "wb");
                        if (f!= NULL){
                            download_file(newSocket, f);
                            printf("File downloaded.\n");
                            fclose(f);
                        } else
                            printf("404 File %s not found\n",filename);
                    } else {
                        printf("File does not exist.\n");
                    }
			}
			if(strcmp(buffer, "put")==0){
				 token = strtok_r(NULL," \n", &t_safe);
                    if( token == NULL ){
                        printf("Usage: put SOURCE_FILENAME\n");
                        continue;
                    }
                    strcpy(filename, token);
                    token = strtok_r(NULL," \n", &t_safe);
                    if( token != NULL ){
                        printf("Usage: put SOURCE FILENAME\n");
                        continue;
                    }
                    printf("Uploading \"%s\"...\n",filename);
                    
                    f = fopen(filename, "rb");
                    if (f != NULL){
                        printf("Sending PUT_REQUEST\n");
                        send_message.type = 0xA9;
                        send_message.protocol[0] = 0xe3;
                        strcat(send_message.protocol, "myftp");
                        memset(send_message.payload, '\0', 1024);
                        memcpy(send_message.payload, filename, strlen(filename));
                        send_message.length = 12 + strlen(filename);
                        send(newSocket,(char *)(&send_message),send_message.length, 0);
                        printf("PUT_REQUEST sent\n");

                        printf("Receiving PUT_REPLY\n");
                        memset(send_message.payload, '\0', 1024);
                        recv(newSocket,(char *)(&send_message),sizeof(struct message_attribute), 0);
                        if( send_type = 0xAA, send_type != send_message.type){
                            printf("Error: Client - Wrong header received, terminating connection");
                            exit(-1);
                        }
                        printf("PUT_REPLY received\n");
                        upload_file(newSocket, f);
                        fclose(f);
                        printf("202 44 Byte %s file retrieved by server and was saved.\n",filename);
                    }
                    else
                        printf("404 File %s is not uploaded.",filename);
			}
			if(strcmp(buffer, "del")==0){
				 token = strtok_r(NULL," \n", &t_safe);
                    if( token == NULL ){
                        printf("Usage: DEL FILENAME\n");
                        continue;
                    }
                    strcpy(filename, token);
                    token = strtok_r(NULL," \n", &t_safe);
                    if( token != NULL ){
                        printf("Usage: DEL FILENAME\n");
                        continue;
                    }
                    printf("DELETED \"%s\"...\n",filename);
                    int result = remove(filename);
                    if(result==0){
                        fprintf(stdout,"file deleted");
                    }
                    else{
                        fprintf(stderr,"404 File %s is not on the server./n",filename);
                    }
			}
		
		}else quit=1;
	}
	send_quit_request(newSocket, &connected);
    close(newSocket);
    printf("Goodbye!\n");
	return 0;
}
