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

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

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
	
	int filesize;
	if(recv_byte(clnt_sock, (void *)&filesize, 4) == 1){ //It makes no sense to receive files of unknown size. Therefore, Server receives filesize from client.
		error_handling("ERROR: receive filesize");
	}
	printf("FILESIZE: %d\n",filesize);
	if(filesize > 50000){ //Assume that Max filesize is 50000bytes. Approximately, 50000bytes is 1000 lines. I guess it is enough as Simple Online C compiler.
		error_handling("ERROR: Too large filedata");
	}
	
	//2. Receive the file data
	FILE *recv_file = fopen(filename, "ab");
	int read_size;
	while( (read_size = read(clnt_sock, buf, BUF_SIZE))/*recv_check = recv(clnt_sock, buf, BUF_SIZE, 0)*/){
		printf("%s",buf);
		if(!fwrite(buf, 1, read_size, recv_file)) error_handling("ERROR: fwrite the filedata with received data.");
	}
	fclose(recv_file);

	//3. gcc compile
	char cmd[256];
	strtok(filename, ".");
	printf("filename: %s.c, filesize: %d\n",filename, filesize);
	sprintf(cmd, "gcc %s.c -o %s 2>&1 ",filename, filename);
	FILE *fp = popen(cmd, "r");
	if(fp == NULL){
		error_handling("ERROR: open pipe stream");
	}
	
	//4. Send result or error message.
	FILE * c_fp = fdopen(dup(fileno(fp)), "r");
	if( pclose(fp)!=0){
		while(fgets(buf, BUF_SIZE, c_fp)){
                	if(send_byte(clnt_sock,buf, BUF_SIZE)) error_handling("ERROR: send the message to client.");
        	}
	}else{
		sprintf(cmd, "./%s", filename);
		FILE * run = popen(cmd, "r");
		if(run == NULL) 
			error_handling("ERROR: open run pipe stream");
		while(fgets(buf, BUF_SIZE, run)){
			if(send_byte(clnt_sock, buf, BUF_SIZE)) error_handling("ERROR: send the result of run");
		}
		sprintf(cmd, "rm %s", filename);
		run = popen(cmd, "r");
		pclose(run);
	}
	
	shutdown(clnt_sock, SHUT_WR);
	
	
	sprintf(cmd, "rm %s.c", filename);
	c_fp = popen(cmd,"r");

	pclose(c_fp);
	close(clnt_sock);
	free(sock);
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
		if (*clnt_sock == -1)
			continue;
		else
			puts("new client connected...");
	
		tid = pthread_create(&tid, NULL, online_compile, clnt_sock);
		pthread_detach(tid);
	}

	close(serv_sock);
	return 0;
}
