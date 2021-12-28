#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	int fd = open(argv[1], O_RDWR), offset = 0;

	for(int idx = 0;idx < strlen(argv[2]);idx++)
	{
		offset = 10 * offset + argv[2][idx] - '0';
	}

	int filesize = lseek(fd, 0, SEEK_END);

	if(offset >= filesize)
	{
		printf("ERROR!\n");
		return 0;
	}

	offset++;

	char *tmp = malloc(filesize * sizeof(char));

	lseek(fd, offset, SEEK_SET);
	read(fd, tmp, filesize - offset);
	lseek(fd, offset, SEEK_SET);
	write(fd, argv[3], strlen(argv[3]));
	write(fd, tmp, filesize - offset);

	close(fd);
}
