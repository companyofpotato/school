#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, char **argv)
{
	int fd = open(argv[1], O_RDWR), offset = 0;

	for(int idx = 0;idx < strlen(argv[2]);idx++)
	{
		offset = offset * 10 + argv[2][idx] - '0';
	}

	if(offset >= lseek(fd, 0, SEEK_END))
	{
		printf("ERROR!\n");
		return 0;
	}

	lseek(fd, offset, SEEK_SET);

	for(int idx = 0;idx < strlen(argv[3]);idx++)
	{
		write(fd, &argv[3][idx], 1);
	}

	close(fd);
}
