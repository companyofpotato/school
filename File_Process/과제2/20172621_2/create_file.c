#include <stdio.h>
#include <stdlib.h>

#define LENGTH 250

int main(int argc, char **argv)
{
	int n = atoi(argv[1]);
	FILE *fp = fopen(argv[2], "wb");

	fwrite(&n, sizeof(n), 1, fp);

	char *buf;
	for(int count = 0;count < n;count++)
	{
		buf = malloc(LENGTH * sizeof(char));
		for(int idx = 0;idx < LENGTH;idx++)
		{
			buf[idx] = count % 10 + '0';
		} 
		buf[249] = '\n';
		fwrite(buf, LENGTH * sizeof(char), 1, fp);
	}

	fclose(fp);

	return 0;
}
