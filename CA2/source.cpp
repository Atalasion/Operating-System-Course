#include <iostream>
#include "csv.h"
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include "log.hpp"

const int MONTH = 12;
const int DAY = 30;
const int HOUR = 6;

using namespace std;

string res;
string source_type;
int p1;
int p2;
char *name;

int init(char *argv[]){
	int cnt = 0;
	p1 = atoi(argv[2]);
	p2 = atoi(argv[3]);
	for (int i = 0; i < strlen(argv[1]); i++){
		if (argv[1][i] == '/') cnt++;
		else if (cnt == 3 && argv[1][i] == '.') break;
		else if(cnt >= 2){
			if (cnt == 3) source_type += argv[1][i];
			res += argv[1][i];
		}
	}
	LOG("source read data from " + res + ".csv");
	source_type[0] = source_type[0] + 'a' - 'A';
	name = const_cast<char*>(res.data());
	return 0;
}

int send_data(char *argv[]){
	io::CSVReader<9> in(argv[1]);
  	in.read_header(io::ignore_extra_column, "Year", "Month", "Day", "0", "1", "2", "3", "4", "5");
 	int year; int month; int day; int d0, d1, d2, d3, d4, d5;
  	LOG("source send data to the house");
  	while(in.read_row(year, month, day, d0, d1, d2, d3, d4, d5)){
 		int line[] = {year, month, day, d0, d1, d2, d3, d4, d5};
 		if (write(p2, line, sizeof(int) * 9) == -1){
 			cout << "Couldn't write on FIFO " << name << '\n';
 		}
 	}
 	return 0;
}

int send_bills(){
	io::CSVReader<5> in_bill("./buildings/bills.csv"); 
 	LOG("source read bills.csv");
 	in_bill.read_header(io::ignore_extra_column, "Year", "Month", "water", "gas", "electricity");
 	int year, month, day, bill, water, gas, electricity;
 	LOG("source send data to the house");
 	while (in_bill.read_row(year, month, water, gas, electricity)){
 		int line[] = {month,  0};
 		if (source_type == "gas") line[1] = gas;
 		else if(source_type == "water") line[1] = water;
 		else line[1] = electricity;
 		if (write(p2, line, sizeof(int) * 2) == -1){
 			cout << "Couldn't write bill on FIFO " << name << '\n'; 
 		}
 	}
 	close(p2);
 	return 0;
}

int main(int argc, char *argv[]){
	init(argv);
	send_data(argv);
	send_bills();
 	exit(0);
 	return 0;
}   

