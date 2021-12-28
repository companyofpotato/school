#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, char **argv)
{
	int fd = open(argv[1], O_RDONLY);	
	int offset = 0, size = 0;
	char *buf;

	for(int idx = 0;idx < strlen(argv[2]);idx++)
	{
		offset = 10 * offset + argv[2][idx] - '0';
	}

	if(offset >= lseek(fd, 0, SEEK_END))
	{
		printf("ERROR!\n");
		return 0;
	}

	for(int idx = 0;idx < strlen(argv[3]);idx++)
	{
		size = 10 * size + argv[3][idx] - '0';
	}

	lseek(fd, offset, SEEK_SET);

	buf = malloc(size * sizeof(char));
	read(fd, buf, size);
	write(1, buf, strlen(buf));

	close(fd);
}
