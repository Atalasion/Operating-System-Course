#include <iostream>
#include <vector>
#include "csv.h"
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <iomanip>
#include <filesystem>
#include <map>
#include "log.hpp"

#define RED_TEXT "\033[1;31m"
#define CYAN_TEXT "\033[1;36m"
#define YELLOW_TEXT "\033[1;33m"
#define GREY_TEXT "\033[1;90m"
#define CREAM_TEXT "\033[38;5;230m"
#define RESET_TEXT "\033[0m"

using namespace std;
namespace fs = filesystem;

map<string, int> c_read_fd;
map<string, int> c_write_fd;
map<string, int> p_read_fd;
map<string, int> p_write_fd;
vector<string> houses;

void sep_line(){
    for (int i = 0; i < 60; i++){
        printf(GREY_TEXT "-" RESET_TEXT);
    }
    printf("\n");
}

void createhouse(string name){
    LOG("main.cpp execl() building.cpp");
    string av = to_string(c_read_fd[name]);
	string dov = to_string(c_write_fd[name]);
	string av2 = to_string(p_read_fd[name]);
	string dov2 = to_string(p_write_fd[name]);
	const char* av_cstr = av.c_str();
	const char* dov_cstr = dov.c_str();
	const char* av2_cstr = av2.c_str();
	const char* dov2_cstr = dov2.c_str();
	if (execl("building.out", "building.out", const_cast<char*>(name.data()), av_cstr, dov_cstr, av2_cstr, dov2_cstr, NULL) == -1){
		cout << "Couldn't Execl " << name << '\n';
	}
}

void send_mes_to_house(string house, int com, string source, int month){
	LOG("main send message to house " + house);
    int arr[] = {com, month};
    if (write(p_write_fd[house], arr, sizeof(int) * 2) == -1){
		cout << "Couldn't write\n";
		return;
	}
	const char *buf = source.c_str();
	if (write(p_write_fd[house], buf, strlen(buf)) == -1){
		cout << "Couldn't write\n";
	}
}

void createbill(){
    LOG("main execl() bills.out");
    if (execl("bills.out", "bills.out", NULL) == -1){
        cout << "Couldn't Execl bill\n";
        return;
    }
}

int create_buildings(){
    string path = "./buildings";
    for (const auto& entry : fs::directory_iterator(path)) {
        if (entry.is_directory()) {
            string name = entry.path().filename();
            houses.push_back(name);
            LOG("create house " + name);
            int pfd[2];
            if (pipe(pfd) == -1){
                cout << "Couldn't make pipe\n";
                return 3;
            }
            p_read_fd[name] = pfd[0];
            p_write_fd[name] = pfd[1];
            int cfd[2];
            if (pipe(cfd) == -1){
                cout << "Couldn't make pipe\n";
                return 3;
            }
            c_read_fd[name] = cfd[0];
            c_write_fd[name] = cfd[1];
            int pid = fork();
            if (pid == -1){
                cout << "Couldn't create fork\n";
                return 2;
            }else if(pid == 0){
                createhouse(name);
            }
        }
    }
    return 0;
}

int create_bill_fifo(){
    string res = "bills_fifo";
    if (mkfifo(const_cast<char*>(res.data()), 0777) == -1){
        if (errno != EEXIST){
            cout << "Couldn't make fifo " << res << '\n';
            return 1;
        }
    }
    res = "bills_r_fifo";
    if (mkfifo(const_cast<char*>(res.data()), 0777) == -1){
        if (errno != EEXIST){
            cout << "Couldn't make fifo " << res << '\n';
            return 1;
        }
    }
    int pid = fork();
    if (pid == -1){
        cout << "Couldn't Execl bills\n";
        return 4;
    }
    if (pid == 0){
        createbill();
    }
    return 0;
}

int handle_request(){
    sep_line();
    printf(YELLOW_TEXT ">>Enter your request number " RESET_TEXT);
    printf(CREAM_TEXT "(Options: 1: average source consumption, 2: total source consumption, 3: source peak hour, 4: get bill, 5: diffrence): " RESET_TEXT);
    int n;
    cin >> n;
    sep_line();
    printf(YELLOW_TEXT ">>Enter the house name " RESET_TEXT);
    printf(CREAM_TEXT "(Options: " RESET_TEXT);
    int sz = houses.size();
    sz--;
    int cnt = 0;
    for (auto u:houses){
        const char * u_c = u.c_str();
        if (cnt == sz) printf(CREAM_TEXT "%s", u_c);
        else printf(CREAM_TEXT "%s, ", u_c);
        cnt++;
    }
    printf("): " RESET_TEXT);
    string house;
    cin >> house;
    sep_line();
    printf(YELLOW_TEXT ">>Enter the source " RESET_TEXT);
    printf(CREAM_TEXT "(Options: Gas, Water, Electricity): " RESET_TEXT);
    string source;
    cin >> source;
    sep_line();
    printf(YELLOW_TEXT ">>Enter the month " RESET_TEXT);
    printf(CREAM_TEXT "(Options: number from 1 to 12): " RESET_TEXT);
    int month;
    cin >> month;
    sep_line();
    send_mes_to_house(house, n, source, month);
    int ans;
    if (read(c_read_fd[house], &ans, sizeof(ans)) == -1){
        cout << "Can't read\n";
        return 4;
    }
    LOG("main received result from " + house);
    for (auto u:c_read_fd) close(u.second);
    for (auto u:c_write_fd) close(u.second);
    for (auto u:p_write_fd) close(u.second);
    for (auto u:p_read_fd) close(u.second);
    printf(CYAN_TEXT "The answer to your request is: " RESET_TEXT);
    printf(RED_TEXT "%d\n", ans);
    sep_line();
    return 0;
}

int main(int argc, char *argv[]){
   	create_buildings();
    create_bill_fifo();
    handle_request();
    return 0;
}
