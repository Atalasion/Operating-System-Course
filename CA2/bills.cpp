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

Source* source;
vector<Data> datas;
vector<pair<int, int>> bil;
int month;
char source_c[128];

int read_data(){
	int fd = open("bills_fifo", O_RDONLY);
	for (int i = 0; i < MONTH * DAY; i++){
		int arr[9];
		if (read(fd, arr, sizeof(int) * 9) == -1){
			cout << "Coudln't read\n";
			return 4;
		}
		Data x;
		x.y = arr[0];
		x.m = arr[1];
		x.d = arr[2];
		for (int j = 0; j < HOUR; j++){
			x.p[j] = arr[j + 3];
		}
		datas.push_back(x);
	}
	for (int i = 0; i < MONTH; i++){
		int arr[2];
		if (read(fd, arr, sizeof(int) * 2) == -1){
			cout << "Coudln't write" << endl;
			return 5;
		}
		bil.push_back({arr[0], arr[1]});
	}
	if (read(fd, &month, sizeof(int)) == -1){
		cout << "Coudln't read\n";
		return 4;
	}
	memset(source_c, 0, sizeof(source_c));
	if (read(fd, source_c, sizeof(source_c)) == -1){
		cout << "Coudln't read\n";
	}
	LOG("bills received data from the house");
	close(fd);
	return 0;
}

int build_source(){
	string source_s(source_c);
	if (source_s == "Gas") source = new Gas();
	if (source_s == "Water") source = new Water();
	if (source_s == "Electricity") source = new Electricity();
	for (auto u:datas){
		source->insert(u);
	}
	for (auto u:bil){
		source->set_bill(u.first, u.second);
	}
	return 0;
}

int write_data(){
	int fd = open("bills_r_fifo", O_WRONLY);
	int res = source->calcbill(month);
	LOG("bills send data to the house");
	if (write(fd,&res, sizeof(int)) == -1){
		return cout << "Couldn't write on bills fifo\n", -1;
	}
	close(fd);
	return 0;
}

int main(int argc, char *argv[]){
	read_data();
	build_source();
	write_data();
}