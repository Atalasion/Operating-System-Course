#include <unistd.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/time.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include "cJSON/cJSON.h"

char buffer[1024] = {0};
int port, tcpsock, tcp_port, sock;
int open_port;
char name[100];
struct sockaddr_in bc_address;
int ispending = 0;

void logFile(const char* msg, const char* username){
    int fd = open("log.txt", O_APPEND | O_CREAT | O_RDWR, 0777);
    write(fd, username, strlen(username));
    write(fd, ": ", 2);
    write(fd, msg, strlen(msg));
    write(fd, "\n", 1);
    close(fd);
}

int setupServer(int port) {
    struct sockaddr_in address, client;
    socklen_t client_len = sizeof(client);
    int server_fd;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    address.sin_port = 0;
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    if (getsockname(server_fd, (struct sockaddr*)&address, &client_len) != -1){
        tcp_port = ntohs(address.sin_port);
    }
    listen(server_fd, 4);

    return server_fd;
}

void print_dot(){
    printf("...................\n");
}

int connectServer(int port){
    int fd;
    struct sockaddr_in server_address;

    fd = socket(AF_INET, SOCK_STREAM, 0);

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0){
        printf("Error in connecting to server\n");
    }

    return fd;
}

int acceptClient(int server_fd) {
    int client_fd = -1;
    struct sockaddr_in client_address;
    int address_len = sizeof(client_address);
    client_fd = accept(server_fd, (struct sockaddr *)&client_address, (socklen_t*) &address_len);
    return client_fd;
}



void send_broadcast(){
    int a = sendto(sock, buffer, strlen(buffer), 0,(struct sockaddr *)&bc_address, sizeof(bc_address));
    if (a <= 0){
        printf("BC Error\n");
    }
    return;
}

void join_broadcast(){
    printf("welcome %s\n", name);
    logFile("jonied server", name);
}

void check_unique(){
    int new_socket = acceptClient(tcpsock);
    if (new_socket != -1){
        printf("username isn't unique\n");
        exit(0);
    }
}

void handle_username(){
    memset(buffer, 0, 1024);
    sprintf(buffer, "join/%s/%d", name, tcp_port);
    send_broadcast();
    signal(SIGALRM, join_broadcast);
    siginterrupt(SIGALRM, 1);
    alarm(2);
    check_unique();
    alarm(0);
}

void init_sup(){
	printf("Please enter your username:\n");
	memset(buffer, 0, 1024);
    int bytes_received = read(0, buffer, 1024);
    memcpy(name, buffer, strlen(buffer) - 1);
    handle_username();
}

int connect_broadcast(int port){
    int sock, broadcast = 1, opt = 1;
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    bc_address.sin_family = AF_INET; 
    bc_address.sin_port = htons(port); 
    bc_address.sin_addr.s_addr = inet_addr("255.255.255.255");

    bind(sock, (struct sockaddr *)&bc_address, sizeof(bc_address));
    return sock;
}

void answer_request(){
	printf("your answer:\n");
	memset(buffer, 0, 1024);
	int bytes_received = read(0, buffer, 1024);
	if (ispending == 0){
		printf("there is no request\n");
		return;
	}
	char comm[100];
	memset(comm, 0, sizeof(comm));
	if (strcmp(buffer, "yes\n") == 0){
		memcpy(comm, "accept", strlen("accept"));
	}
	else if (strcmp(buffer, "no\n") == 0){
		memcpy(comm, "reject", strlen("reject"));
	}else{
		printf("invalid command\n");
		return;
	}
	int fd = connectServer(open_port);
	send(fd, comm, strlen(comm), 0);
	ispending = 0;
}

void handle_stdin(){
	if (strcmp(buffer, "answer request\n") == 0){
		answer_request();
	}else{
        printf("invalid command\n");
        return;
    }
    logFile(buffer, name);
}

void control_duplicate(){
    char tmp[100];
    int port;
    char strport[10];
    int koj;
    for (int i = 0; i < strlen(buffer) - 5; i++){
        if (buffer[i + 5] == '/'){
            koj = i + 5 + 1;
            break;
        }
        tmp[i] = buffer[i + 5];
    }
    memcpy(strport, buffer + koj, strlen(buffer) - koj);
    port = atoi(strport);
    if (port != tcp_port && strcmp(tmp, name) == 0){
        connectServer(port);
    }
}

void send_supplier(){
	char strport[100];
	memset(strport, 0, sizeof(strport));
	memcpy(strport, buffer + 14, strlen(buffer) - 14);
	int port = atoi(strport);
	int fd = connectServer(port);
	char tmp[1000];
	memset(tmp, 0, sizeof tmp);
	sprintf(tmp, "supplier/%s/%d", name, tcp_port);
	send(fd, tmp, strlen(tmp), 0);
}

void handle_broadcast(){
    char *comm = (char *)malloc(strlen(buffer) * sizeof(char));
    memset(comm, 0, sizeof(comm));
    memcpy(comm, buffer, 5);
    if (strcmp(comm, "join/") == 0){
        control_duplicate();
        return;
    }
    memset(comm, 0, sizeof(comm));
    memcpy(comm, buffer, strlen("get_suppliers/"));
    if (strcmp(comm, "get_suppliers/") == 0){
    	send_supplier();
    }
}

void handle_tcp(){
	printf("%s\n", buffer);
	if (strcmp(buffer, "Time out") == 0){
		printf("request expired\n");
		ispending = 0;
		return;
	}
	if (ispending == 1){
		return;
	}
	printf("new request\n");
	ispending = 1;
	char strport[10];
	memset(strport, 0, sizeof(strport));
	int koj = 0;
	for (int i = 0; i < strlen(buffer); i++){
		if (buffer[i] == '/'){
			koj = i + 1;
			continue;
		}
	}
	memcpy(strport, buffer + koj, strlen(buffer) - koj);
	open_port = atoi(strport);
}

int main(int argc, char const *argv[]){
	port = atoi(argv[1]);
	sock = connect_broadcast(port);
	tcpsock = setupServer(port);
	init_sup();
	fd_set master_set, working_set;
    FD_ZERO(&master_set);
    int max_sd = tcpsock;
    FD_SET(sock, &master_set);
    FD_SET(0, &master_set);
    FD_SET(tcpsock, &master_set);
	while (1) {
        working_set = master_set;
        select(max_sd + 1, &working_set, NULL, NULL, NULL);
        if (FD_ISSET(0, &working_set)){
            memset(buffer,0, sizeof(buffer));
            int bytes_received = read(0, buffer, sizeof(buffer));
            handle_stdin();
        }
        else if(FD_ISSET(sock, &working_set)){
            memset(buffer, 0, 1024);
            int bytes_received = recv(sock, buffer, 1024, 0);
            handle_broadcast();
        }
        for (int i = 0; i <= max_sd; i++) {
            if (i == sock || i == 0) continue;
            if (FD_ISSET(i, &working_set)) {
                
                if (i == tcpsock) { 
                    int new_socket = acceptClient(tcpsock);
                    FD_SET(new_socket, &master_set);
                    if (new_socket > max_sd)
                        max_sd = new_socket;
                }
                
                else {
                    int bytes_received;
                    memset(buffer, 0, 1024);
                    bytes_received = recv(i , buffer, 1024, 0);
                    if (bytes_received == 0) { // EOF
                        close(i);
                        FD_CLR(i, &master_set);
                        continue;
                    }
                    handle_tcp();                    
                }
            }
        }

    }
}