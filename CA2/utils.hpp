#ifndef __UTILS_HPP__
#define __UTILS_HPP__

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
#include "log.hpp"

const int MONTH = 12;
const int DAY = 30;
const int HOUR = 6;

using namespace std;

struct Data{
	int y, m, d;
	int p[6]; 
};

class Source{
protected:
	vector<int> bills;
	vector<Data> datas;
public:
	void insert(Data data);
	double mean(int mon);
	int tot_p(int mon);
	int peak_time(int mon);
	double diff(int mon);
	void set_bill(int mon, int x);
	int getbill(int mon, char* source);
	virtual double calcbill(int mon) = 0;
};
class Gas:public Source{
public:
	double calcbill(int mon);
};
class Water:public Source{
public:
	double calcbill(int mon);
};
class Electricity:public Source{
public:
	double calcbill(int mon);
};


#endif