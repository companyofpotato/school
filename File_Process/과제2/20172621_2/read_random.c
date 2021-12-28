#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>

#define SUFFLE_NUM 10000
#define LENGTH 250

void GenRecordSequence(int *, int );
void swap(int *, int *);

int main(int argc, char **argv)
{
	FILE *fp = fopen(argv[1], "rb");
	struct timeval start_t, end_t;

	gettimeofday(&start_t, NULL);


	int num_of_records;
	fread(&num_of_records, 4, 1, fp);

	int *read_order_list = malloc(num_of_records * sizeof(int));

	GenRecordSequence(read_order_list, num_of_records);

	char *buf = malloc(LENGTH * sizeof(char));
	for(int idx = 0;idx < num_of_records;idx++)
	{
		fseek(fp, 4 + read_order_list[idx] * LENGTH, SEEK_SET);
		fread(buf, LENGTH * sizeof(char), 1, fp);
	}

	gettimeofday(&end_t, NULL);

	printf("\n#records: %d elapsed_time: %ld us\n", num_of_records, (end_t.tv_sec - start_t.tv_sec) * 1000000 + end_t.tv_usec - start_t.tv_usec);

	return 0;
}

void GenRecordSequence(int *list, int n)
{
	int i, j, k;

	srand((unsigned int)time(0));

	for(i = 0;i < n;i++)
	{
		list[i] = i;
	}

	for(i = 0;i < SUFFLE_NUM;i++)
	{
		j = rand() % n;
		k = rand() % n;
		swap(&list[j], &list[k]);
	}
}

void swap(int *a, int *b)
{
	int tmp;
	tmp = *a;
	*a = *b;
	*b = tmp;
}
