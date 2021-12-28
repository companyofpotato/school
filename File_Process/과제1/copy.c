#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#define BUF_SIZE 10

int main(int argc, char **argv)
{
	int old_fd = open(argv[1], O_RDONLY);
	int new_fd = open(argv[2], O_RDWR|O_CREAT);
	char buf[BUF_SIZE];
	int read_size = BUF_SIZE;

	lseek(old_fd, 0, SEEK_SET);

	while(read_size == BUF_SIZE)
	{
		read_size = read(old_fd, buf, BUF_SIZE);
		write(new_fd, buf, read_size);
	}

	close(old_fd);
	close(new_fd);
}
