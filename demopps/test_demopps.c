#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>

#define SIZE 32
int main(int argc, char *argv[])
{
	int fd = 0, ret = 0;
	off_t offset = 32;
	char buf[SIZE] = {0};
	int i = 0;

	fd = open("/dev/dpsctl0", O_RDWR);
	if (fd < 0){
		perror("Opening /dev/dpsctl");
		return -1;
	}

	while(i < 100){
		ret = read(fd, buf, SIZE);
		if(ret < 0){
			perror("Read error");
		}
		printf("Reading\n");
		i++;
	}
#if 0
	ret = pread(fd, buf, SIZE, offset);
	if(ret < 0){
		perror("Read error");
	}
#endif
	close(fd);

	return 0;
}
