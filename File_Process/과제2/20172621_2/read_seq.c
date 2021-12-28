#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define LENGTH 250

int main(int argc, char **argv)
{
	struct timeval start_t, end_t;
	FILE *fp = fopen(argv[1], "rb");

	gettimeofday(&start_t, NULL);

	int n;
	fread(&n, sizeof(n), 1, fp);

	char *buf = malloc(LENGTH * sizeof(char));
	for(int count = 0;count < n;count++)
	{
		fread(buf, LENGTH * sizeof(char), 1, fp);
	}

	gettimeofday(&end_t, NULL);


	printf("\n#records: %d elapsed_time: %ld us\n", n, (end_t.tv_sec - start_t.tv_sec) * 1000000 + end_t.tv_usec - start_t.tv_usec);

	return 0;
}
