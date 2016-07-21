#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define SIZE 32
int main(int argc, char *argv[])
{
	int fd = 0, ret = 0;
	off_t offset = 32;
	char buf[SIZE] = {0};
	fd = open("/dev/dpsctl0", O_RDWR);
	if (fd < 0){
		perror("Opening /dev/dpsctl");
		return -1;
	}

	ret = read(fd, buf, SIZE);
	if(ret < 0){
		perror("Read error");
	}
	ret = pread(fd, buf, SIZE, offset);
	if(ret < 0){
		perror("Read error");
	}
	close(fd);

	return 0;
}
