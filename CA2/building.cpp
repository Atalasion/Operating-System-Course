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
#include <map>
#include "utils.hpp"
#include "log.hpp"

using namespace std;

Gas gas;
Water water;
Electricity electricity;

void handle_gas(string name, int p1[2]){
	string res = name + "Gas";
	string addrs = "./buildings/" + name + "/Gas.csv";
	char* nameres = const_cast<char*>(res.data()); 
	string av_s = to_string(p1[0]);
	string dov_s = to_string(p1[1]);
	const char* av = av_s.c_str();
	const char* dov = dov_s.c_str();
	LOG("house " + name + "execl() Gas source");
	if (execl("source.out", "source.out", const_cast<char*>(addrs.data()), av, dov, NULL) == -1){
		cout << "Can't execl " << res << '\n';
		return;
	}
	exit(0);
}

void handle_water(string name, int p2[2]){
	string res = name + "Water";
	string addrs = "./buildings/" + name + "/Water.csv";
	char* nameres = const_cast<char*>(res.data()); 
	string av_s = to_string(p2[0]);
	string dov_s = to_string(p2[1]);
	const char* av = av_s.c_str();
	const char* dov = dov_s.c_str();
	LOG("house " + name + "execl() Water source");
	if (execl("source.out", "source.out", const_cast<char*>(addrs.data()), av, dov, NULL) == -1){
		cout << "Can't execl " << res << '\n';
		return;
	}
	exit(0);
}

void handle_electricity(string name, int p3[2]){
	string res = name + "Electricity";
	string addrs = "./buildings/" + name + "/Electricity.csv";
	char* nameres = const_cast<char*>(res.data()); 
	string av_s = to_string(p3[0]);
	string dov_s = to_string(p3[1]);
	const char* av = av_s.c_str();
	const char* dov = dov_s.c_str();
	LOG("house " + name + "execl() Electricity source");
	if (execl("source.out", "source.out", const_cast<char*>(addrs.data()), av, dov, NULL) == -1){
		cout << "Can't execl " << res << '\n';
		return;
	}
	exit(0);
}
string name;

void get_gas_datas(int p1[2]){
	string nameres = "Gas Pipe";
	for (int i = 0; i < MONTH * DAY; i++){
		int arr[9];
		if (read(p1[0], arr, sizeof(int) * 9) == -1){
			cout << "Can't read " << nameres << '\n';
			return;
		}
		Data tmp;
		tmp.y = arr[0];
		tmp.m = arr[1];
		tmp.d = arr[2];
		for (int j = 0; j < 6; j++) tmp.p[j] = arr[j + 3];
		gas.insert(tmp);
	}
	for (int i = 0; i < MONTH; i++){
		int arr[2];
		if (read(p1[0], arr, sizeof(int) * 2) == -1){
			cout << "Can't read " << nameres << '\n';
			return;
		}
		gas.set_bill(arr[0], arr[1]);
	}
	LOG("house " + name + "received Gas.csv and bills.csv data");
	close(p1[0]);
}

void get_water_datas(int p2[2]){
	string nameres = "Water Pipe";
	for (int i = 0; i < MONTH * DAY; i++){
		int arr[9];
		if (read(p2[0], arr, sizeof(int) * 9) == -1){
			cout << "Can't read " << nameres << '\n';
			return;
		}
		Data tmp;
		tmp.y = arr[0];
		tmp.m = arr[1];
		tmp.d = arr[2];
		for (int j = 0; j < 6; j++) tmp.p[j] = arr[j + 3];
		water.insert(tmp);
	}
	for (int i = 0; i < MONTH; i++){
		int arr[2];
		if (read(p2[0], arr, sizeof(int) * 2) == -1){
			cout << "Can't read " << nameres << '\n';
			return;
		}
		water.set_bill(arr[0], arr[1]);
	}
	LOG("house " + name + "received Water.csv and bills.csv data");	
	close(p2[0]);
}

void get_electricity_datas(int p3[2]){
	string nameres = "Electricity Pipe";
	for (int i = 0; i < MONTH * DAY; i++){
		int arr[9];
		if (read(p3[0], arr, sizeof(int) * 9) == -1){
			cout << "Can't read " << nameres << '\n';
			return;
		}
		Data tmp;
		tmp.y = arr[0];
		tmp.m = arr[1];
		tmp.d = arr[2];
		for (int j = 0; j < 6; j++) tmp.p[j] = arr[j + 3];
		electricity.insert(tmp);
	}
	for (int i = 0; i < MONTH; i++){
		int arr[2];
		if (read(p3[0], arr, sizeof(int) * 2) == -1){
			cout << "Can't read " << nameres << '\n';
			return;
		}
		electricity.set_bill(arr[0], arr[1]);
	}
	LOG("house " + name + "received Electricity.csv and bills.csv data");
	close(p3[0]);
}

int get_command(string name, int parent_fd[2], int child_fd[2]){
	int com, month, arr[2];
	if (read(parent_fd[0], arr, 2 * sizeof(int)) == -1){
		cout << "Couldn't read\n";
		return 1;
	}
	com = arr[0];
	month = arr[1];
	char source_c[128];
	memset(source_c, 0, sizeof(source_c));
	if (read(parent_fd[0], source_c, sizeof(source_c)) == -1){
		cout << "Couldn't read\n";
		return 2;
	}
	LOG("house " + name + "received the request");
	string source(source_c);
	int res;
	map<string, Source*> sources = {
        {"Gas", &gas},
        {"Water", &water},
        {"Electricity", &electricity}
    };
    switch(com){
		case 1:
			res = sources[source]->mean(month);
			break;
		case 2:
			res = sources[source]->tot_p(month);
			break;
		case 3:
			res = sources[source]->peak_time(month);
			break;
		case 4:
			res = sources[source]->getbill(month, source_c);
			break;
		case 5:
			res = sources[source]->diff(month);
			break;
	}
	LOG("house " + name + "answered the request");
	if (write(child_fd[1], &res, sizeof(int)) == -1){
		cout << "Couldn't write\n";
		return 4;
	}
	return 0;
}

int p1[2], p2[2], p3[2];
int parent_fd[2];
int child_fd[2];


int init(char *argv[]){
	parent_fd[0] = atoi(argv[4]); // parent to child
	parent_fd[1] =  atoi(argv[5]);
	child_fd[0] = atoi(argv[2]); // child to parent
	child_fd[1] = atoi(argv[3]);
	if (pipe(p1) == -1){return 1;}
	if (pipe(p2) == -1) {return 2;}
	if (pipe(p3) == -1) {return 3;}
	name = argv[1];
	int id = fork();
	if (id == -1){
		cout << "Error while " << name << "Gas\n";
	}
	if (id == 0){
		handle_gas(argv[1], p1);
	}else{
		id = fork();
		if (id == -1){
			cout << "Error while " << name << ' ' << "Electricity\n";
		}else if(id == 0){
			handle_electricity(argv[1], p3);
		}else{
			id = fork();
			if (id == -1){
				cout << "Error while " << name << ' ' << "Water\n";
			}else if(id == 0){
				handle_water(argv[1], p2);
			}
		}
	}
	return 0;
}

int main(int argc, char *argv[]){
	init(argv);
	wait(0);
	wait(0);
	wait(0);
	get_gas_datas(p1);
	get_water_datas(p2);
	get_electricity_datas(p3);
	get_command(name, parent_fd, child_fd);	
}