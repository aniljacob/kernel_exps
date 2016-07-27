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
	fd = open("/dev/pgwlk0", O_RDWR);
	if (fd < 0){
		perror("Opening /dev/pgwalk");
		return -1;
	}

	ret = read(fd, buf, SIZE);
	if(ret < 0){
		perror("Read error");
	}
	//if offset want to be specified.
#if 0
	ret = pread(fd, buf, SIZE, offset);
	if(ret < 0){
		perror("Read error");
	}
#endif
	close(fd);

	/*need a while loop to inspect 'cat /proc/<pid>/map' file
	 * to see our app and map are showing the same memory maps
	 * */
	while(1);

	return 0;
}
