
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#define BUF_SIZE 1024

int recv_byte(int sock, void *buf, size_t len){
	ssize_t recv_check;
	int total_size = len;
	char *buf_p = buf; 
	while( total_size > 0 && (recv_check = recv(sock, buf_p, total_size, 0))){
		buf_p += recv_check;
		total_size -= recv_check;
	}
	if(recv_check < 0) return 1;

	return 0;
}

int send_byte(int sock, void *buf, size_t len){
	ssize_t send_check;
	int total_size = len;
	char *buf_p = buf; 
	while( total_size > 0 && (send_check = send(sock, buf_p, total_size, 0))){
		buf_p += send_check;
		total_size -= send_check;
	}
	if(send_check < 0) return 1;

	return 0;
}

void *online_compile(void *sock){
	int clnt_sock = *((int *)sock);
	char buf[BUF_SIZE];
	
	ssize_t recv_check;
	int total_size;
	//1. Receive the filename(255bytes) and filesize(4bytes)
	if(recv_byte(clnt_sock, buf, 255) == 1){
		error_handling("ERROR: receive filename");
	}
	char filename[255];//Max filename length in LINUX
	strcpy(filename, buf);
	buf = 0x0;
	if(recv_byte(clnt_sock, buf, 4) == 1){ //It makes no sense to receive files of unknown size. Therefore, Server receives filesize from client.
		error_handling("ERROR: receive filesize");
	}
	int filesize = atoi(buf);
	if(filesize > 50000){ //Assume that Max filesize is 50000bytes. Approximately, 50000bytes is 1000 lines. I guess it is enough as Simple Online C compiler.
		error_handling("ERROR: Too large filedata");
	}
	
	//2. Receive the file data
	buf = 0x0;
	FILE *recv_file = fopen(filename, "w");
	while(recv(clnt_sock, buf, filesize, 0)){
		if(!fwrite(buf, 1, filesize, recv_file)) error_handling("ERROR: fwrite the filedata with received data.");
	}
	fclose(recv_file);


	//3. gcc compile
	char cmd[256];
	char object[256] = strtok(filename, ".");
	sprintf(cmd, "gcc %s -o %s 2>&1",filename, object);
	FILE *fp = popen(cmd, "r");
	if(fp == NULL){
		error_handling("ERROR: open pipe stream");
	}
	//4. result or error message sends.

	while(fgets(buf, BUF_SIZE, fp)){
		if(send(clnt_sock,buf, BUF_SIZE)) error_handling("ERROR: send the message to client.");
	}
	shutdown(clnt_sock, SHUT_WR);

	if(recv_byte(clnt_sock, buf, 10)) error_handling("ERROR: receive Thank you"); 
	//sprintf(cmd, "rm %s", filename);
	//fp = popen(cmd,r);
	pclose(fp);
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}


int main(int argc, char *argv[])
{
	int serv_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	
	pid_t pid;
	struct sigaction act;
	socklen_t adr_sz;
	int str_len, state;
	char buf[BUF_SIZE];
	if (argc != 2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}

	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));
	
	if (bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr)) == -1)
		error_handling("bind() error");
	if (listen(serv_sock, 5) == -1)
		error_handling("listen() error");
	
	pthread_t tid;
	while (1)
	{
		adr_sz = sizeof(clnt_adr);
		int *clnt_sock = malloc(sizeof(int));
		*clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz);
		if (clnt_sock == -1)
			continue;
		else
			puts("new client connected...");
	
		tid = pthread_create(&tid, NULL, online_compile, clnt_sock);
		pthread_detach(tid);
		close(*clnt_sock);
		free(clnt_sock);
	}

	close(serv_sock);
	return 0;
}
