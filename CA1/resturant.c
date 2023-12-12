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

#define MAX_ING 1000

typedef struct{
    char name[100];
    int port;
}supplier;

struct food_request{
    char name[1000];
    int port;
    char order[1000];
    char status[20];
};

struct food{
    char *name;
    struct ingredient* ing;
    int tot_ing;
};

struct ingredient{
    char *name;
    int amount;
};

struct resturant{
    char* name;
    int open;
    struct ingredient* ing;
    struct food* foods;
    int tot_ing;
    int tot_food;
};


struct resturant rest;
supplier suppliers[MAX_ING];
int tot_sup, tcp_port;

volatile sig_atomic_t while_flag = 1;  

struct food_request food_requests[100];
int tot_req = 0;

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
    // address.sin_port = htons(port);
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

char buffer[1024] = {0};
struct sockaddr_in bc_address;
int sock, tcpsock;

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

void add_food_ing(char *name, char *ing, int amount){
    int f = 0;
    for (int i = 0; i < rest.tot_food; i++){
        if (strcmp(rest.foods[i].name, name) == 0){
            rest.foods[i].ing = (struct ingredient*) realloc(rest.foods[i].ing, (rest.foods[i].tot_ing + 1) * sizeof(struct ingredient));
            struct ingredient ings;
            ings.name = (char *)malloc(1024*sizeof(char));
            memcpy(ings.name, ing, strlen(ing));
            ings.amount = amount;
            rest.foods[i].ing[rest.foods[i].tot_ing] = ings;
            rest.foods[i].tot_ing++;
            f = 1;
            break;
        }
    }
    if (f == 0){
        struct food foods;
        foods.ing = (struct ingredient*)malloc(1*sizeof(struct ingredient));
        foods.ing[0].name = (char *)malloc(1024*sizeof(char));
        memcpy(foods.ing[0].name, ing, strlen(ing));
        foods.ing[0].amount = amount;
        foods.tot_ing = 1;
        foods.name = (char*)malloc(1024*sizeof(char));
        memcpy(foods.name, name, strlen(name));
        rest.foods[rest.tot_food] = foods;
        rest.tot_food ++;
    }
  //  printf("%s %s %d %d %s\n", name, ing, amount, rest.tot_food, rest.foods[0].name);
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
        add_food_ing(food_name, ing, a);
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




void close_res(){
    if (rest.open == 0){
        printf("resturant is already closed\n");
    }
    int f = 0;
    for (int i = 0; i < tot_req; i++){
        if (strcmp(food_requests[i].status, "pending") == 0){
            f = 1;
        }
    }
    if (f){
        printf("there are some requests\n");
        return;
    }
    rest.open = 0;
}

void open_res(){
    if (rest.open == 1){
        printf("resturant is already open\n");
    }
    rest.open = 1;
}

void join_broadcast(){
    printf("welcome %s\n", rest.name);
    logFile("joined server", rest.name);
    while_flag = 0;
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
    sprintf(buffer, "join/%s/%d", rest.name, tcp_port);
    send_broadcast();
    signal(SIGALRM, join_broadcast);
    siginterrupt(SIGALRM, 1);
    alarm(2);
    check_unique();
    alarm(0);
}

void init_res(){
    printf("Please enter your username:\n");
    memset(buffer, 0, 1024);
    int bytes_received = read(0, buffer, 1024);
    if (bytes_received == 0){
        close_res();
    }
    rest.name = (char*)malloc(strlen(buffer) * sizeof(char));
    memcpy(rest.name, buffer, strlen(buffer) - 1);
    handle_username();
    rest.open = 1;
    rest.ing = (struct ingredient*)malloc(MAX_ING * sizeof(struct ingredient));
    rest.foods = (struct food*) malloc(MAX_ING * sizeof(struct food));
    rest.tot_ing = 0;
    rest.tot_food = 0;
}

void show_ingredients(){
    if (rest.open == 0){
        printf("Resturant is closed\n");
        return;
    }
    printf("...................\n");
    printf("ingredients/amount\n");
    for (int i = 0; i < rest.tot_ing; i++){
        if (rest.ing[i].amount != 0){
            printf("%s %d\n", rest.ing[i].name, rest.ing[i].amount);
        }
    }
    printf("...................\n");
}

void show_recipes(){
    if (rest.open == 0){
        printf("Resturant is closed\n");
        return;
    }
    printf("...................\n");
    for (int i = 0; i < rest.tot_food; i++){
        printf("%d:\n", i + 1);
        printf("\t%s:\n", rest.foods[i].name);
        for (int j = 0; j < rest.foods[i].tot_ing; j++){
            printf("\t\t%s : %d\n", rest.foods[i].ing[j].name, rest.foods[i].ing[j].amount);
        }
    }
    printf("...................\n");
}

void modify(){
    while_flag = 0;
}

void check_suppliers(){
    while (1){
        int new_socket = acceptClient(tcpsock);
        if (new_socket == -1) break;
        int bytes_received;
        memset(buffer, 0, 1024);
        bytes_received = recv(new_socket, buffer, 1024, 0);
        char comm[100];
        memset(comm, 0, sizeof comm);
        memcpy(comm, buffer, 9);

        if (strcmp("supplier/", comm) == 0){
            char username[100];
            int koj;
            for (int i = 9; i < strlen(buffer); i++){
                if (buffer[i] == '/'){
                    koj = i + 1;
                    break;
                }
                username[i - 9] = buffer[i];
            }
            char strport[10];
            memcpy(strport, buffer + koj, strlen(buffer) - koj);
            int port = atoi(strport);
            memset(suppliers[tot_sup].name, 0, sizeof(suppliers[tot_sup].name));
            memcpy(suppliers[tot_sup].name, username, strlen(username));
            suppliers[tot_sup].port = port;
            tot_sup ++;    
        }
    }
}

void get_suppliers(){
    tot_sup = 0;
    struct sigaction sa;
    signal(SIGALRM, modify);
    siginterrupt(SIGALRM, 1);
    alarm(2);
    check_suppliers();
    alarm(0);
}

void show_suppliers(){
    if (rest.open == 0){
        printf("Resturant is closed\n");
        return;
    }
    memset(buffer, 0, 1024);
    sprintf(buffer, "get_suppliers/%d", tcp_port);
    send_broadcast();
    get_suppliers();
    printf("...................\n");
    printf("username/port\n");
    for (int i = 0; i < tot_sup; i++){ 
        printf("%s %d\n", suppliers[i].name, suppliers[i].port);
    }
    printf("...................\n");
}
int handler_port;

void ingredient_handler(){
    printf("Time out\n");
    int fd = connectServer(handler_port);
    send(fd, "Time out", 8, 0);
}

void add_ingredient(char name[100], int amount){
    int f = 1;
    for (int i = 0; i < rest.tot_ing; i++){
        if (strcmp(rest.ing[i].name, name) == 0){
            rest.ing[i].amount += amount;
            f = 0;
        }
    }
    if (f){
        struct ingredient ing;
        ing.name = (char*)malloc(100 * sizeof(char));
        memcpy(ing.name, name, strlen(name));
        ing.amount = amount;
        rest.ing[rest.tot_ing ++] = ing;
    }
}

void accept_ingredient(char name[100], int amount){
    printf("supplier accepted\n");
    add_ingredient(name, amount);
}

void reject_ingredient(char name[100], int amount){
    printf("supplier rejected\n");
}

void request_ingredient(){
    printf("port of supplier:\n");
    memset(buffer,0, 1024);
    int bytes_received = read(0, buffer, 1024);
    memcpy(buffer, buffer, strlen(buffer) - 1);
    int port = atoi(buffer);
    printf("name of ingredient\n");
    memset(buffer, 0, 1024);
    bytes_received = read(0, buffer, 1024);
    char ingredient_name[100];
    memset(ingredient_name, 0, sizeof(ingredient_name));
    memcpy(ingredient_name, buffer, strlen(buffer) - 1);
    printf("amount of ingredient\n");
    memset(buffer, 0, 1024);
    bytes_received = read(0, buffer, 1024);
    memcpy(buffer, buffer, strlen(buffer) - 1);
    int amount = atoi(buffer);
    int fd = connectServer(port);
    memset(buffer, 0, 1024); 
    sprintf(buffer, "get ingredient/%d", tcp_port);
    send(fd, buffer, strlen(buffer), 0);
    printf("waiting for supplier response\n");
    handler_port = port;
    signal(SIGALRM, ingredient_handler);
    siginterrupt(SIGALRM, 1);
    alarm(90);
    int new_socket = acceptClient(tcpsock);
    alarm(0);
    if (new_socket >= 0){
        memset(buffer, 0, 1024);
        bytes_received = recv(new_socket , buffer, 1024, 0);
        if (strcmp("accept", buffer) == 0){
            accept_ingredient(ingredient_name, amount);
        }else if(strcmp("reject", buffer) == 0){
            reject_ingredient(ingredient_name, amount);
        }
    }
}

void show_requests_list(){
    print_dot();
    printf("username/port/order\n");
    for (int i = 0; i < tot_req; i++){
        if (strcmp(food_requests[i].status, "pending") == 0){
            printf("%s/%d/%s\n", food_requests[i].name, food_requests[i].port, food_requests[i].order);
        }
    }
    print_dot();
}

void timeout_request(){
    char strport[20];
    memcpy(strport, buffer + 9, strlen(buffer) - 9);
    int port = atoi(strport);
    for (int i = 0; i < tot_req; i++){
        if (food_requests[i].port == port){
            memcpy(food_requests[i].status, "denied", 6);
            return;
        }
    }
    return;
}

void request_accepted(int port){
    int fd = connectServer(port);
    memset(buffer, 0, 1024);
    memcpy(buffer, "accepted", 8);
    send(fd, buffer, strlen(buffer), 0);
}

void request_denied(int port){
    int fd = connectServer(port);
    memset(buffer, 0, 1024);
    memcpy(buffer, "denied", 6);
    send(fd, buffer, strlen(buffer), 0);
}

void accept_request(int port){
    int pos = -1;
    int koj;
    for (int i = 0; i < tot_req; i++){
        if (food_requests[i].port == port && strcmp(food_requests[i].status, "pending") == 0){
            for (int j = 0; j < rest.tot_food; j++){
                if (strcmp(rest.foods[j].name, food_requests[i].order) == 0){
                    pos = j;
                koj = i;
                }
            }
            break;
        }
    }
    if (pos > -1){
        int f = 1;
        for (int i = 0; i < rest.foods[pos].tot_ing; i++){
            int ff = 0;
            for (int j = 0; j < rest.tot_ing; j++){
                if (strcmp(rest.ing[j].name, rest.foods[pos].ing[i].name) == 0 && rest.ing[j].amount >= rest.foods[pos].ing[i].amount){
                    ff = 1;
                }
            }
            f *= ff;
        }
        if (f){
            memset(food_requests[koj].status, 0, sizeof(food_requests[koj].status));
            memcpy(food_requests[koj].status, "accepted", 8);        
            // printf("%d %s\n", pos, food_requests[pos].status);
            request_accepted(port);
            for (int i = 0; i < rest.foods[pos].tot_ing; i++){
                for (int j = 0; j < rest.tot_ing; j++){
                    if (strcmp(rest.ing[j].name, rest.foods[pos].ing[i].name) == 0 && rest.ing[j].amount >= rest.foods[pos].ing[i].amount){
                        rest.ing[i].amount -= rest.foods[pos].ing[i].amount;    
                    }
                }
            }   
            return;
        }else{
            printf("there isn't enough ingredient\n");
            memset(food_requests[koj].status, 0, sizeof(food_requests[koj].status));
            memcpy(food_requests[koj].status, "denied", 6);        
            request_denied(port);
            return;
        }
    }
    printf("there isn't any request with that port\n");
    return;
}

void denied_request(int port){
    for (int i = 0; i < tot_req; i++){
        if (food_requests[i].port == port && strcmp(food_requests[i].status, "pending") == 0){
            memset(food_requests[i].status, 0, sizeof(food_requests[i].status));
            memcpy(food_requests[i].status, "denied", 6);
            request_denied(port);
            return;
        }
    }
    printf("there isn't any request with that port\n");
    return;
}

void answer_request(){
    printf("port of request:\n");
    memset(buffer, 0, 1024);
    int bytes_received = read(0, buffer, 1024);
    int port = atoi(buffer);
    printf("your answer:\n");
    memset(buffer, 0, 1024);
    bytes_received = read(0, buffer, 1024);
    if (strcmp("yes\n", buffer) == 0){
        accept_request(port);
    }
    else if (strcmp("no\n", buffer) == 0){
        denied_request(port);
    }else{
        printf("invalid command\n");
    }   
}

void show_sales_history(){
    print_dot();
    printf("username/order/result\n");
    for (int i = 0; i < tot_req; i++){
        if (strcmp(food_requests[i].status, "pending") != 0){
            printf("%s %s %s\n", food_requests[i].name, food_requests[i].order, food_requests[i].status);
        }
    }
    print_dot();
}

void handle_stdin(){
    char comm[1000];
    memset(comm, 0, sizeof(comm));
    memcpy(comm, buffer, strlen(buffer));
    if (strcmp(comm, "start working\n") == 0){
        open_res();
    }
    else if (strcmp(comm, "break\n") == 0){
        close_res();
    }else if(rest.open == 0){
        printf("Resturant is closed\n");
        return;
    }else if (strcmp(comm, "show ingredients\n") == 0){
        show_ingredients();
    }
    else if (strcmp(comm, "show recipes\n") == 0){
        show_recipes();
    }
    else if (strcmp(comm, "show suppliers\n") == 0){
        show_suppliers();
    }
    else if (strcmp(comm, "request ingredient\n") == 0){
        request_ingredient();
    }
    else if (strcmp(comm, "show requests list\n") == 0){
        show_requests_list();
    }
    else if (strcmp(comm, "answer request\n") == 0){
        answer_request();
    }else if(strcmp(comm, "show sales history\n") == 0){
        show_sales_history();
    }else{
        printf("invalid command\n");
        return;
    }
    logFile(comm, rest.name);
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
    if (port != tcp_port && strcmp(tmp, rest.name) == 0){
        connectServer(port);
    }
}

void send_restruants(){
    char strport[10];
    memcpy(strport, buffer + 15, strlen(buffer) - 15);
    int port = atoi(strport);
    int fd = connectServer(port);
    memset(buffer, 0, 1024);
    sprintf(buffer, "resturant/%s/%d", rest.name, tcp_port);
    send(fd, buffer, strlen(buffer), 0);
}

void handle_broadcast(){
    char *comm = (char *)malloc(strlen(buffer) * sizeof(char));
    memset(comm, 0, sizeof(comm));
    memcpy(comm, buffer, 5);
    if (rest.open == 0){
        printf("Resturant is closed\n");
        return;
    }
    if (strcmp(comm, "join/") == 0){
        control_duplicate();
    }
    memset(comm, 0, sizeof(comm));
    memcpy(comm, buffer, 15);
    if (strcmp(comm, "get_resturants/") == 0){
        send_restruants();
    }
}

void add_request(char name[1000], char order[1000], int port){
    memcpy(food_requests[tot_req].name, name, strlen(name));
    food_requests[tot_req].port = port;
    memcpy(food_requests[tot_req].order, order, strlen(order));
    memcpy(food_requests[tot_req].status, "pending", 7);
    tot_req++;
}

void handle_food_request(){
    printf("new request\n");
    char name[1000];
    memset(name, 0, sizeof(name));
    int koj = 0;
    for (int i = 13; i < strlen(buffer); i++){
        if (buffer[i] == '/'){
            koj = i + 1;
            break;
        }
        name[i - 13] = buffer[i];
    }
    char order[1000];
    memset(order, 0, sizeof(order));
    for (int i = koj; i < strlen(buffer); i++){
        if (buffer[i] == '/'){
            koj = i + 1;
            break;
        }
        order[i - koj] = buffer[i];
    }
    char strport[100];
    memcpy(strport, buffer + koj, strlen(buffer) - koj);
    int port = atoi(strport);
    add_request(name, order, port);
}

void handle_tcp(){
    char comm[1000];
    memset(comm, 0, sizeof(comm));
    memcpy(comm, buffer, 12);
    if (rest.open == 0){
        return;
    }
    if (strcmp(comm, "food_request") == 0){
        handle_food_request();
        return;
    }
    memset(comm, 0, sizeof comm);
    memcpy(comm, buffer, 9);
    if (strcmp(buffer, "Time out/") == 0){
        timeout_request();
        return;
    }
}

int main(int argc, char const *argv[]) {
    int port = atoi(argv[1]);
    sock = connect_broadcast(port);
    tcpsock = setupServer(port);
    init_res();
    cJSON *json;
    json = read_json();
    int new_socket, max_sd;
    fd_set master_set, working_set;
    FD_ZERO(&master_set);
    max_sd = tcpsock;
    FD_SET(sock, &master_set);
    FD_SET(0, &master_set);
    FD_SET(tcpsock, &master_set);
    while (1) {
        working_set = master_set;
        select(max_sd + 1, &working_set, NULL, NULL, NULL);
        if (FD_ISSET(0, &working_set)){
            memset(buffer,0, 1024);
            int bytes_received = read(0, buffer, 1024);
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
                
                if (i == tcpsock) {  // new clinet
                    new_socket = acceptClient(tcpsock);
                    FD_SET(new_socket, &master_set);
                    if (new_socket > max_sd)
                        max_sd = new_socket;
                }
                
                else {
                    int bytes_received;
                    memset(buffer, 0, 1024);
                    bytes_received = recv(i , buffer, 1024, 0);
                    if (bytes_received == 0) { // EOF
                        // printf("client fd = %d closed\n", i);
                        close(i);
                        FD_CLR(i, &master_set);
                        continue;
                    }
                    handle_tcp();                    
                }
            }
        }

    }
        
    return 0;
}