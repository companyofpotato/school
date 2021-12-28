#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include "blockmap.h"

static int amt[DATABLKS_PER_DEVICE];
static int freepbn = 0;

void ftl_get_information(int *free_block_number, int *address_mapping_table)
{
	int i;
	*free_block_number = freepbn;
	for(i = 0;i < DATABLKS_PER_DEVICE;i++)
	{
		address_mapping_table[i] = amt[i];
	}
}

void ftl_open()
{
	char *pagebuf = malloc(PAGE_SIZE * sizeof(char));
	int lbn;
	
	for(int i = 0;i < DATABLKS_PER_DEVICE;i++)
	{
		amt[i] = -1;
	}

	for(int i = 0;i < DATABLKS_PER_DEVICE;i++)
	{
		dd_read(i * PAGES_PER_BLOCK, pagebuf);
		memcpy(&lbn, pagebuf + SECTOR_SIZE, sizeof(int));
		if(lbn >= 0)
			amt[lbn] = i;
	}

	free(pagebuf);

	while(1)
	{
		int i = 0;
		for(;i < DATABLKS_PER_DEVICE;i++)
		{
			if(amt[i] == freepbn)
			{
				freepbn++;
				break;
			}
		}
		if(i == DATABLKS_PER_DEVICE)
			break;
	}
	return;
}

void ftl_read(int lsn, char *sectorbuf)
{
	int lbn = lsn / PAGES_PER_BLOCK;
	int pbn = amt[lbn];
	if(pbn < 0)
	{
		printf("no block\n");
		exit(0);
	}
	int offset = lsn % PAGES_PER_BLOCK;
	int ppn = pbn * PAGES_PER_BLOCK + offset;
	char *pagebuf = malloc(PAGE_SIZE * sizeof(char));
	dd_read(ppn, pagebuf);
	int oldlsn;
	memcpy(&oldlsn, pagebuf + SECTOR_SIZE + 4, sizeof(int));
	
	if(oldlsn >= 0)
	{
		memcpy(sectorbuf, pagebuf, SECTOR_SIZE * sizeof(char));
		free(pagebuf);
	}
	else
	{
		printf("no page\n");
		exit(0);
	}
	return;
}

void ftl_write(int lsn, char *sectorbuf)
{
	int lbn = lsn / PAGES_PER_BLOCK;
	int pbn = amt[lbn];
	if(pbn < 0)
	{
		while(1)
		{
			pbn++;
			if(pbn == freepbn)
				continue;
			int i = 0;
			for(;i < DATABLKS_PER_DEVICE;i++)
			{
				if(pbn == amt[i])
					break;
			}
			if(i == DATABLKS_PER_DEVICE)
				break;
		}
		amt[lbn] = pbn;
		char *tmp = malloc(PAGE_SIZE * sizeof(char));
		dd_read(pbn * PAGES_PER_BLOCK, tmp);
		memcpy(tmp + SECTOR_SIZE, &lbn, sizeof(int));
		dd_write(pbn * PAGES_PER_BLOCK, tmp);
		free(tmp);
	}
	int offset = lsn % PAGES_PER_BLOCK;
	int ppn = pbn * PAGES_PER_BLOCK + offset;
	char *pagebuf = malloc(PAGE_SIZE * sizeof(char));
	dd_read(ppn, pagebuf);

	int oldlsn;
	memcpy(&oldlsn, pagebuf + SECTOR_SIZE + 4, sizeof(int));

	if(oldlsn >= 0)
	{
		char *tmp = malloc(PAGE_SIZE * sizeof(char));
		int tmplsn;

		dd_read(pbn, tmp);
		for(int i = 0; i < PAGES_PER_BLOCK;i++)
		{
			dd_read(pbn * PAGES_PER_BLOCK + i, tmp);
			memcpy(&tmplsn, tmp + SECTOR_SIZE + 4, sizeof(int));
			if(tmplsn == oldlsn)
			{
				memcpy(tmp, sectorbuf, SECTOR_SIZE * sizeof(char));
			}
			dd_write(freepbn * PAGES_PER_BLOCK + i, tmp);
		}

		free(tmp);

		amt[lbn] = freepbn;
		freepbn = pbn;
		dd_erase(freepbn);
	}
	else
	{
		memcpy(pagebuf, sectorbuf, SECTOR_SIZE * sizeof(char));
		memcpy(pagebuf + SECTOR_SIZE, &lbn, sizeof(int));
		memcpy(pagebuf + SECTOR_SIZE + 4, &lsn, sizeof(int));
		dd_write(ppn, pagebuf);
	}
	free(pagebuf);

	return;
}

void ftl_print()
{
	for(int i = 0;i < DATABLKS_PER_DEVICE;i++)
	{
		printf("%d %d\n", i, amt[i]);
	}
	printf("free block's pbn=%d\n", freepbn);
	return;
}
