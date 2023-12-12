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

char buffer[1024] = {0}, name[100];
struct sockaddr_in bc_address;
int sock, tcpsock, tcp_port, tot_rest;

void logFile(const char* msg, const char* username){
    int fd = open("log.txt", O_APPEND | O_CREAT | O_RDWR, 0777);
    write(fd, username, strlen(username));
    write(fd, ": ", 2);
    write(fd, msg, strlen(msg));
    write(fd, "\n", 1);
    close(fd);
}

struct resturant{
	char name[100];
	int port;
};

struct food{
	char name[100];
};

int tot_food, handler_port;
struct food foods[1024];

struct resturant rest[1024];

void print_dot(){
    printf("...................\n");
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

void send_broadcast(){
    int a = sendto(sock, buffer, strlen(buffer), 0,(struct sockaddr *)&bc_address, sizeof(bc_address));
    if (a <= 0){
        printf("BC Error\n");
    }
    return;
}

void join_broadcast(){
    printf("welcome %s\n", name);
    logFile("joined server", name);
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

void init_cust(){
	printf("Please enter your username:\n");
	memset(buffer, 0, 1024);
    int bytes_received = read(0, buffer, 1024);
    memcpy(name, buffer, strlen(buffer) - 1);
    handle_username();
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

void handle_broadcast(){
	char *comm = (char *)malloc(strlen(buffer) * sizeof(char));
    memset(comm, 0, sizeof(comm));
    memcpy(comm, buffer, 5);
    if (strcmp(comm, "join/") == 0){
        control_duplicate();
        return;
    }
}


void modify(){
	int flag = 0;
}
void check_resturants(){
    while (1){
        int new_socket = acceptClient(tcpsock);
        if (new_socket == -1) break;
        int bytes_received;
        memset(buffer, 0, 1024);
        bytes_received = recv(new_socket, buffer, 1024, 0);
        char comm[100];
        memset(comm, 0, sizeof comm);
        memcpy(comm, buffer, 10);
		if (strcmp("resturant/", comm) == 0){
            char username[100];
            int koj;
            for (int i = 10; i < strlen(buffer); i++){
                if (buffer[i] == '/'){
                    koj = i + 1;
                    break;
                }
                username[i - 10] = buffer[i];
            }
            char strport[10];
            memcpy(strport, buffer + koj, strlen(buffer) - koj);
            int port = atoi(strport);
            memset(rest[tot_rest].name, 0, sizeof(rest[tot_rest].name));
            memcpy(rest[tot_rest].name, username, strlen(username));
            rest[tot_rest].port = port;
            tot_rest ++;    
        }
    }
}


void get_resturants(){
    tot_rest = 0;
    struct sigaction sa;
    signal(SIGALRM, modify);
    siginterrupt(SIGALRM, 1);
    alarm(2);
    check_resturants();
    alarm(0);
}

void show_resturants(){
	memset(buffer, 0, 1024);
	sprintf(buffer, "%s/%d\n", "get_resturants", tcp_port);
	send_broadcast();
	get_resturants();
	print_dot();
	printf("username/port\n");
	for (int i = 0; i < tot_rest; i++){
		printf("%s %d\n", rest[i].name, rest[i].port);
	}
	print_dot();
}

void show_menu(){
	print_dot();
	for (int i = 0; i < tot_food; i++){
		printf("%d: %s\n", i + 1, foods[i].name);
	}
	print_dot();
}

void response_handler(){
	printf("Time out\n");
	int fd = connectServer(handler_port);
	send(fd, "Time out", 8, 0);
}

void response_wait(int port){
	printf("waiting for resturant response\n");
    handler_port = port;
    signal(SIGALRM, response_handler);
    siginterrupt(SIGALRM, 1);
    alarm(120);
    int new_socket = acceptClient(tcpsock);
    alarm(0);
    if (new_socket >= 0){
        memset(buffer, 0, 1024);
        int bytes_received = recv(new_socket , buffer, 1024, 0);
        if (strcmp("accepted", buffer) == 0){
            printf("order accepted\n");
        }else if(strcmp("denied", buffer) == 0){
            printf("order denied\n");
        }
    }
}

void order_food(){
	printf("name of food:\n");
	memset(buffer, 0, 1024);
	read(0, buffer, 1024);
	char food_name[100];
	printf("port of resturant\n");
	memset(food_name, 0, sizeof(food_name));
	memcpy(food_name, buffer, strlen(buffer) - 1);
	memset(buffer, 0, 1024);
	read(0, buffer, 1024);
	memcpy(buffer, buffer, strlen(buffer) - 1);
	int port = atoi(buffer);
	memset(buffer, 0, 1024);
	printf("%s %d\n", food_name, port);
	sprintf(buffer, "food_request/%s/%s/%d", name, food_name, tcp_port);
	int fd = connectServer(port);
	send(fd, buffer, strlen(buffer), 0);
	response_wait(port);
}

void handle_stdin(){
	if (strcmp(buffer, "show resturants\n") == 0){
		show_resturants();
	}
	else if (strcmp(buffer, "show menu\n") == 0){
		show_menu();
	}else if(strcmp(buffer, "order food\n") == 0){
		order_food();
	}else{
		printf("invalid command\n");
		return;
	}
	logFile(buffer, name);
}

void add_food(char food_name[1024]){
	for (int i = 0; i < tot_food; i++){
		if (strcmp(foods[i].name, food_name) == 0){
			return;
		}
	}
	memcpy(foods[tot_food ++].name, food_name, strlen(food_name));
}

void traverseJSON(cJSON *json, const char *parentKey) {
    if (cJSON_IsObject(json)) {
        cJSON *child = json->child;
        while (child) {
            char newKey[100]; 
            if (parentKey != NULL) {
                snprintf(newKey, sizeof(newKey), "%s.%s", parentKey, child->string);
            } else {
                snprintf(newKey, sizeof(newKey), "%s", child->string);
            }
            traverseJSON(child, newKey);
            child = child->next;
        }
    } else if (cJSON_IsArray(json)) {
        cJSON *child = json->child;
        int i = 0;
        while (child) {
            char newKey[100];
            snprintf(newKey, sizeof(newKey), "%s[%d]", parentKey, i);
            traverseJSON(child, newKey);
            child = child->next;
            i++;
        }
    } else {
        char food_name[1024], ing[1024], amount[1024];
        memset(food_name, '\0', 1024);
        memset(ing, '\0', 1024);
        int koj = -1;
        for (int i = 0; i < strlen(parentKey); i++){
            if (parentKey[i] == '.'){
                koj = i + 1;
                continue;
            }
            if (koj == -1){
                food_name[i] = parentKey[i];
            }else{
                ing[i - koj] = parentKey[i];
            }
        }
        int a = atoi(cJSON_Print(json));
        add_food(food_name);
    }
}


cJSON* read_json(){
    FILE *file = fopen("recipes.json", "r");
   
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *json_buffer = (char *)malloc(file_size + 1);
    fread(json_buffer, 1, file_size, file);
    json_buffer[file_size] = '\0';
    fclose(file);

    cJSON *json = cJSON_Parse(json_buffer);
    traverseJSON(json, NULL);
}

int main(int argc, char const *argv[]) {
    int port = atoi(argv[1]);
    sock = connect_broadcast(port);
    tcpsock = setupServer(port);
    init_cust();
    cJSON *json;
    json = read_json();
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
    }
}