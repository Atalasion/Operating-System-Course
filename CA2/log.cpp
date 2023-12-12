#include "log.hpp"

void LOG(string msg){
	const char* msg_c = msg.c_str();
	int fd = open("log.txt", O_APPEND|O_CREAT|O_WRONLY, 0777);
	write(fd, "-- ", 3);
	write(fd, msg_c, strlen(msg_c));
	write(fd, "\n", 1);
	close(fd);
}