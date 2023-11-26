
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>



#define BUF_SIZE 1024
#define FILENAME 255
void error_handling(char *message);

int send_byte(int sock, void *buf, size_t len){
        ssize_t send_check;
        int total_size = len;
        char *buf_p = buf;
        while( total_size > 0 && 0 < (send_check = send(sock, buf_p, total_size, 0))){
                buf_p += send_check;
                total_size -= send_check;
        }
        if(send_check < 0) return 1;
        return 0;
}
int main(int argc, char *argv[])
{
	int sd;
	FILE *fp;
	
	char file_name[255];
	char buf[BUF_SIZE];
	int read_cnt;
	int read_size;
	struct sockaddr_in serv_adr;
	if (argc != 4) {
		printf("Usage: %s <IP> <port> <file name> \n", argv[0]);
		exit(1);
	}
	
	fp = fopen(argv[3], "rb");
	sd = socket(PF_INET, SOCK_STREAM, 0);   

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_adr.sin_port = htons(atoi(argv[2]));

	connect(sd, (struct sockaddr*)&serv_adr, sizeof(serv_adr));
	
	//1. Send file name 
	strcpy(file_name, argv[3]);
	if(send_byte(sd, file_name, FILENAME)) error_handling("ERROR: send filename");
	
	//2. Send file size
	struct stat st;
	char filepath[1024];
	sprintf(filepath, "./%s",file_name);
	if(stat(filepath, &st) == -1){
		error_handling("ERROR: stat()");
	}
	int filesize = st.st_size;
	if(send_byte(sd, (void *)&filesize, sizeof(int))) error_handling("ERROR: send filesize");

	//3. Send file data
	fp = fopen(file_name, "r");
	
	while( (read_size = fread(buf, 1, BUF_SIZE, fp))){
		if(!write(sd, buf, read_size)/*send_byte(sd, buf, sizeof(buf))*/) error_handling("ERROR: send file data");
	}
	shutdown(sd, SHUT_WR);

	//4. Receive compile result.
	while(recv(sd, buf, BUF_SIZE, 0)){
		printf("%s",buf);
	}

	fclose(fp);
	close(sd);
	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
