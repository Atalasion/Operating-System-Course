#include "utils.hpp"
void Source::insert(Data data){
	datas.push_back(data);
}

void Source::set_bill(int mon, int x){
	while (bills.size() != MONTH) bills.push_back(0);
	bills[mon] = x;
}

double Source::mean(int mon){
	double sum = 0.0;
	double cnt = 0.0;
	for (auto u:datas){
		if (u.m == mon){
			for (int j = 0; j < HOUR; j++){
				sum += u.p[j];
				cnt += 1.0;
			}
		}
	} 
	return sum / cnt;
}

int Source::tot_p(int mon){
	int tot_m = 0;
	for (auto u:datas){
		if (u.m != mon) continue;
		for (int i = 0; i < HOUR; i++){
			tot_m += u.p[i];
		}
	}
	return tot_m;
}

int Source::peak_time(int mon){
	vector<int> tot_m(HOUR, 0);
	for (auto u:datas){
		if (u.m != mon) continue;
		for (int i = 0; i < HOUR; i++){
			tot_m[i] += u.p[i];
		}
	}
	int mx = 0;
	for (int i = 1; i < HOUR; i++){
		if (tot_m[i] > tot_m[mx]) mx = i; 
	}
	return mx;
}

double Source::diff(int mon){
	double mean = this->mean(mon);
	int peak_hour = this->peak_time(mon);
	double sum = 0.0;
	double cnt = 0.0; 
	for (auto u:datas){
		if (u.m != mon) continue;
		sum += u.p[peak_hour];
		cnt += 1.0;
	}
	return sum / cnt - mean;
}

int Source::getbill(int mon, char* source){
	int fd = open("bills_fifo", O_WRONLY);
	for (auto u:datas){
		int arr[9];
		arr[0] = u.y;
		arr[1] = u.m;
		arr[2] = u.d;
		for (int j = 0; j < 6; j++) arr[3 + j] = u.p[j];
		if (write(fd, arr, sizeof(int) * 9) == -1){
			cout << "Couldn't write\n";
			return -1;
		}
	}

	for (int i = 0; i < MONTH; i++){
		int arr[] = {i, bills[i]};
		if (write(fd, arr, sizeof(int) * 2) == -1){
			return cout << "Couldn't write on bills fifo\n", -1;	
		}	
	}
	if (write(fd, &mon, sizeof(int)) == -1){
		return cout << "Couldn't write on bills fifo\n", -1;
	}
	if (write(fd, source, strlen(source)) == -1){
		return cout << "Couldn't write on bills fifo\n", -1;	
	}
	close(fd);
	fd = open("bills_r_fifo", O_RDONLY);
	int ans;
	if (read(fd, &ans, sizeof(int)) == -1){
		return cout << "Couldn't read on bills fifo\n", -1;
	}
	close(fd);
	return ans;
}


double Water::calcbill(int mon){
	double sum = 0;
	int peak_hour = this->peak_time(mon);
	for (auto u:datas){
		if (u.m != mon) continue;
		for (int j = 0; j < HOUR; j++){
			if (j == peak_hour) sum += u.p[j] * bills[u.m] * 1.25;
			else sum += u.p[j] * bills[u.m];
		}
	}
	return sum;
}



double Electricity::calcbill(int mon){
	double sum = 0;
	double mean = this->mean(mon);
	int peak_hour = this->peak_time(mon);
	for (auto u:datas){
		if (u.m != mon) continue;
		for (int j = 0; j < HOUR; j++){
			if (j == peak_hour) sum += u.p[j] * bills[u.m] * 1.25;
			else if(u.p[j] < mean) sum += u.p[j] * bills[u.m] * 0.75;
			else sum += u.p[j] * bills[u.m];
		}
	}
	return sum;
}

double Gas::calcbill(int mon){
	double sum = 0;
	for (auto u:datas){
		if (u.m != mon) continue;
		for (int j = 0; j < HOUR; j++){
			sum += u.p[j] * bills[u.m];
		}
	}
	return sum;
}
