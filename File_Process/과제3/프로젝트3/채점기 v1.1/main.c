#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "blockmap.h"

#define PAGES_PER_DEVICE (PAGES_PER_BLOCK * BLOCKS_PER_DEVICE)
#define FLASH_DRIVE_SIZE (BLOCK_SIZE * BLOCKS_PER_DEVICE)

FILE *flashfp;

void ftl_open();
void ftl_write(int lsn, char *sectorbuf);
void ftl_read(int lsn, char *sectorbuf);
void ftl_print();
void ftl_get_information(int *free_block_number, int *address_mapping_table);
int dd_read(int ppn, char *pagebuf);

static int calculate();
static void initialize_file();
static int ftl_open_check_1();
static int ftl_open_check_2();
static int ftl_write_check_1();
static int ftl_write_check_2();
static int ftl_read_check_1();
static int ftl_read_check_2();

int main(int argc, char *argv[])
{
	printf("최종 점수 : %d / 100\n", calculate());
	return 0;
}

static int calculate()
{
	int score;

	score = 0;
	initialize_file();

	score += ftl_open_check_1();  // 10
	score += ftl_write_check_1(); // 30
	score += ftl_read_check_1();  // 10
	score += ftl_write_check_2(); // 30
	score += ftl_read_check_2();  // 10
	score += ftl_open_check_2();  // 10

	fclose(flashfp);

	return score;
}

static void clear_flashmemory(char *file_name)
{
	char *blockbuf;
	int i;

	flashfp = fopen(file_name, "w+b");
	if (flashfp == NULL)
	{
		printf("file open error\n");
		exit(1);
	}
	blockbuf = (char *)malloc(BLOCK_SIZE);
	memset(blockbuf, 0xFF, BLOCK_SIZE);
	for (i = 0; i < BLOCKS_PER_DEVICE; ++i)
		fwrite(blockbuf, BLOCK_SIZE, 1, flashfp);
	free(blockbuf);
}

static int validate_data(char *buffer, int data)
{
	int i;
	for (i = 0; i < SECTOR_SIZE; ++i)
		if (buffer[i] != (char)data)
			return -1;
	return 0;
}

static int validate_sector(int ppn, int data)
{
	char pagebuf[PAGE_SIZE];
	dd_read(ppn, pagebuf);
	return validate_data(pagebuf, data);
}

static int validate_empty_blocks(int amt[DATABLKS_PER_DEVICE])
{
	int filled[BLOCKS_PER_DEVICE] = {0};
	int i, j;
	for (i = 0; i < DATABLKS_PER_DEVICE; ++i)
	{
		if (0 <= amt[i] && amt[i] < BLOCKS_PER_DEVICE)
		{
			filled[amt[i]] = TRUE;
		}
	}
	for (i = 0; i < BLOCKS_PER_DEVICE; ++i)
	{
		if (!filled[i])
		{
			for (j = 0; j < PAGES_PER_BLOCK; ++j)
			{
				if (validate_sector(i * PAGES_PER_BLOCK + j, 0xFF))
				{
					printf("%d번 block의 %d번 page가 비어있지 않습니다.\n", i, j);
					return -1;
				}
			}
		}
	}
	return 0;
}

static int validate_spare(int ppn, int lsn)
{
	char pagebuf[PAGE_SIZE];
	int lbn;
	int spare_lbn, spare_lsn;
	int ret;
	dd_read(ppn, pagebuf);
	lbn = lsn / PAGES_PER_BLOCK;
	spare_lbn = *((int *)(pagebuf + SECTOR_SIZE));
	spare_lsn = *((int *)(pagebuf + SECTOR_SIZE + 4));
	ret = 0;
	if (lbn != spare_lbn)
	{
		printf("이상 발견. spare.lbn 값이 %d가 아닙니다.\n", lbn);
		printf("spare.lbn 값은 %d 입니다.\n", spare_lbn);
		ret = -1;
	}
	if (lsn != spare_lsn)
	{
		printf("이상 발견. spare.lsn 값이 %d가 아닙니다.\n", lsn);
		printf("spare.lsn 값은 %d 입니다.\n", spare_lsn);
		ret = -1;
	}
	return ret;
}

static void print_line()
{
	printf("------------------------------------------------\n\n");
}

static void initialize_file()
{
	int i;
	char *blockbuf;

	printf("flashmemory 파일 초기화...\n");
	clear_flashmemory("flashmemory");
	printf("flashmemory 파일 초기화 성공\n\n");
	print_line();
}

static int ftl_open_check_1() // 총 10점. pfb 2점, amt 3점, 데이터 5점
{
	int score;
	int amt[DATABLKS_PER_DEVICE], pfb;
	int i, flag;

	// ftl_open
	printf("ftl_open() 실행\n\n");
	ftl_open();

	// validate pfb
	printf("free block's pbn 값이 정상적으로 초기화가 되었는지 확인...\n");
	ftl_get_information(&pfb, amt);
	if (0 <= pfb && pfb < BLOCKS_PER_DEVICE)
	{
		printf("free block's pbn 이상 없음. 값은 %d\n\n", pfb);
		score += 2;
	}
	else
	{
		printf("이상 발견. free block's pbn의 값이 블록 인덱스를 가리키지 않습니다.\n");
		printf("free block's pbn의 값이 %d입니다. -2점\n\n", pfb);
	}

	// validate amt
	printf("address mapping table이 전부 -1로 초기화가 되었는지 확인...\n");
	flag = TRUE;
	for (i = 0; i < DATABLKS_PER_DEVICE; ++i)
	{
		if (amt[i] != -1)
		{
			flag = FALSE;
			break;
		}
	}
	if (flag)
	{
		printf("address mapping table 이상 없음.\n\n");
		score += 3;
	}
	else
	{
		printf("이상 발견. address mapping table의 값이 -1로 초기화되지 않았습니다.\n");
		printf("table[%d]의 값이 %d입니다. -3점\n\n", i, amt[i]);
	}

	// validate sector
	printf("flashmemory 파일이 수정되었는지 확인...\n");
	if (!validate_empty_blocks(amt))
	{
		printf("flashmemory 파일 이상 없음.\n\n");
		score += 5;
	}
	else
	{
		printf("이상 발견. ftl_open() 이후 flashmemory 파일이 수정되었습니다.\n");
		printf("%d번 physical page가 훼손되었습니다. -5점\n\n", i);
	}

	printf("ftl_open() 점수 : %d/10\n\n", score);
	print_line();

	return score;
}

static int ftl_write_check_1() // 총 30점 : 쓰기 10점 3번
{
	int score;
	int tc, td[3] = {PAGES_PER_BLOCK, PAGES_PER_BLOCK * 2 + 1, PAGES_PER_BLOCK * 2 + 2};
	int i;
	int amt[DATABLKS_PER_DEVICE], pfb;
	int amt_cmp[DATABLKS_PER_DEVICE];
	char sectorbuf[SECTOR_SIZE];
	int before_lbn, lbn, pbn;
	int flag;

	score = 0;
	before_lbn = 1;

	// ftl_write
	for (tc = 0; tc < 3; ++tc)
	{
		printf("ftl_write(%d, 0x%d%d...) 실행\n\n", td[tc], tc, tc);
		memset(sectorbuf, tc * 17, sizeof(sectorbuf));
		ftl_write(td[tc], sectorbuf);

		lbn = td[tc] / PAGES_PER_BLOCK;
		printf("address mapping table[%d] 값 확인...\n", lbn);
		ftl_get_information(&pfb, amt);
		pbn = amt[td[tc] / PAGES_PER_BLOCK];
		if (0 <= pbn && pbn < BLOCKS_PER_DEVICE && pbn != pfb)
		{
			printf("table[%d] 값 이상 없음. 값은 %d\n\n", lbn, pbn);
			score += 2;
		}
		else
		{
			printf("이상 발견. ftl_write(%d, 0x%d%d...) 실행 이후 table[%d] 값이 정상적으로 갱신되지 않았습니다.\n", td[tc], tc, tc, lbn);
			printf("table[%d] 값이 %d입니다. -2점\n\n", lbn, pbn);
		}

		printf("나머지 address mapping table 값 확인...\n", lbn);
		flag = TRUE;
		for (i = 0; i < DATABLKS_PER_DEVICE; ++i)
		{
			if (i != lbn && i != before_lbn && amt[i] != -1)
			{
				flag = FALSE;
				break;
			}
		}
		if (flag)
		{
			printf("table 값 이상 없음.\n\n");
			score += 2;
		}
		else
		{
			printf("이상 발견. 나머지 table 값이 -1이 아닙니다.\n");
			printf("table[%d] 값이 %d입니다. -2점\n\n", i, amt[i]);
		}

		printf("free block's pbn 값 확인...\n");
		flag = TRUE;
		for (i = 0; i < DATABLKS_PER_DEVICE; ++i)
		{
			if (pfb == amt[i])
			{
				flag = FALSE;
				break;
			}
		}
		if (flag && 0 <= pfb && pfb < BLOCKS_PER_DEVICE)
		{
			printf("free block's pbn 값 이상 없음. 값은 %d\n\n", pfb);
			score += 2;
		}
		else
		{
			printf("이상 발견. free block's pbn 값에 문제가 있습니다.\n");
			printf("free block's pbn 값이 %d입니다. -2점\n\n", pfb);
		}

		printf("%d번 block의 %d번 page에 정상적으로 데이터를 썼는지 확인...\n", pbn, td[tc] % PAGES_PER_BLOCK);
		if (!validate_sector(pbn * PAGES_PER_BLOCK + td[tc] % PAGES_PER_BLOCK, tc * 17))
		{
			printf("page 데이터 이상 없음.\n\n");
			score += 1;
		}
		else
		{
			printf("이상 발견. page 데이터의 값이 0x%d%d가 아닙니다. -1점\n\n", tc, tc);
		}

		printf("%d번 block의 %d번 page에 정상적으로 spare를 작성했는지 확인...\n", pbn, td[tc] % PAGES_PER_BLOCK);
		if (!validate_spare(pbn * PAGES_PER_BLOCK + td[tc] % PAGES_PER_BLOCK, td[tc]))
		{
			printf("page spare 이상 없음.\n\n");
			score += 2;
		}
		else
		{
			printf("-2점\n\n");
		}

		printf("다른 block에는 데이터를 쓰지 않았는지 확인...\n");
		if (!validate_empty_blocks(amt))
		{
			printf("다른 block 이상 없음.\n\n");
			score += 1;
		}
		else
		{
			printf("-1점\n\n");
		}
		printf("\n");
	}

	printf("ftl_write() 점수 : %d/30\n\n", score);
	print_line();

	return score;
}

static int ftl_read_check_1() // 총 10점
{
	int score;
	int tc, td[3] = {PAGES_PER_BLOCK, PAGES_PER_BLOCK * 2 + 1, PAGES_PER_BLOCK * 2 + 2};
	int amt_org[DATABLKS_PER_DEVICE], pfb_org;
	int amt[DATABLKS_PER_DEVICE], pfb;
	char sectorbuf[SECTOR_SIZE];
	int flag;
	int i;

	score = 1;

	ftl_get_information(&pfb_org, amt_org);

	// ftl_write
	for (tc = 0; tc < 3; ++tc)
	{
		printf("ftl_read(%d, sectorbuf) 실행\n\n", td[tc]);
		ftl_read(td[tc], sectorbuf);
		ftl_get_information(&pfb, amt);

		printf("데이터를 온전히 읽었는지 확인...\n");
		if (!validate_data(sectorbuf, tc * 17))
		{
			printf("데이터 이상 없음.\n\n", pfb);
			score += 1;
		}
		else
		{
			printf("이상 발견. 데이터의 값에 문제가 있습니다.\n");
			printf("sector[0]의 값이 0x%2X입니다. -1점\n\n", sectorbuf[0]);
		}

		printf("free block's pbn이 바뀌지 않았는지 확인...\n");
		if (pfb_org == pfb)
		{
			printf("free block's pbn 값 이상 없음.\n\n", pfb);
			score += 1;
		}
		else
		{
			printf("이상 발견. free block's pbn 값이 변경되었습니다.\n");
			printf("free block's pbn 값이 %d에서 %d로 변경되었습니다. -1점\n\n", pfb_org, pfb);
		}

		printf("address mapping table이 바뀌지 않았는지 확인...\n");
		flag = TRUE;
		for (i = 0; i < DATABLKS_PER_DEVICE; ++i)
		{
			if (amt_org[i] != amt[i])
			{
				flag = FALSE;
				break;
			}
		}
		if (flag)
		{
			printf("address mapping table 값 이상 없음.\n\n", pfb);
			score += 1;
		}
		else
		{
			printf("이상 발견. address mapping table이 변경되었습니다.\n");
			printf("table[%d] 값이 %d에서 %d로 변경되었습니다. -1점\n\n", i, amt_org[i], amt[i]);
		}
		printf("\n");
	}

	printf("ftl_read() 점수 : %d/10\n\n", score);
	print_line();

	return score;
}

static int ftl_write_check_2() // 총 30점 : 쓰기 10점 3번
{
	int score;
	int tc, td[3] = {PAGES_PER_BLOCK, PAGES_PER_BLOCK * 2 + 1, PAGES_PER_BLOCK * 2 + 2};
	int i;
	int amt[DATABLKS_PER_DEVICE], pfb;
	int amt_cmp[DATABLKS_PER_DEVICE];
	char sectorbuf[SECTOR_SIZE];
	int lbn, pbn;
	int flag;

	score = 0;

	// ftl_write
	for (tc = 0; tc < 3; ++tc)
	{
		ftl_get_information(&pfb, amt_cmp);

		printf("ftl_write(%d, 0x%d%d...) 실행\n\n", td[tc], tc + 3, tc + 3);
		memset(sectorbuf, (tc + 3) * 17, sizeof(sectorbuf));
		ftl_write(td[tc], sectorbuf);

		lbn = td[tc] / PAGES_PER_BLOCK;
		printf("address mapping table[%d] 값 확인...\n", lbn);
		ftl_get_information(&pfb, amt);
		pbn = amt[td[tc] / PAGES_PER_BLOCK];
		if (0 <= pbn && pbn < BLOCKS_PER_DEVICE && pbn != pfb)
		{
			printf("table[%d] 값 이상 없음. 값은 %d\n\n", lbn, pbn);
			score += 2;
		}
		else
		{
			printf("이상 발견. ftl_write(%d, 0x%d%d...) 실행 이후 table[%d] 값이 정상적으로 갱신되지 않았습니다.\n", td[tc], tc + 3, tc + 3, lbn);
			printf("table[%d] 값이 %d입니다. -2점\n\n", lbn, pbn);
		}

		printf("나머지 address mapping table 값 확인...\n", lbn);
		flag = TRUE;
		for (i = 0; i < DATABLKS_PER_DEVICE; ++i)
		{
			if (i != lbn && amt[i] != amt_cmp[i])
			{
				flag = FALSE;
				break;
			}
		}
		if (flag)
		{
			printf("table 값 이상 없음.\n\n");
			score += 2;
		}
		else
		{
			printf("이상 발견. 나머지 table 값이 변경되었습니다.\n");
			printf("table[%d] 값이 %d에서 %d로 변경되었습니다. -2점\n\n", i, amt_cmp[i], amt[i]);
		}

		printf("free block's pbn 값 확인...\n");
		flag = TRUE;
		for (i = 0; i < DATABLKS_PER_DEVICE; ++i)
		{
			if (pfb == amt[i])
			{
				flag = FALSE;
				break;
			}
		}
		if (flag && 0 <= pfb && pfb < BLOCKS_PER_DEVICE)
		{
			printf("free block's pbn 값 이상 없음. 값은 %d\n\n", pfb);
			score += 2;
		}
		else
		{
			printf("이상 발견. free block's pbn 값에 문제가 있습니다.\n");
			printf("free block's pbn 값이 %d입니다. -2점\n\n", pfb);
		}

		printf("%d번 block의 %d번 page에 정상적으로 데이터를 썼는지 확인...\n", pbn, td[tc] % PAGES_PER_BLOCK);
		if (!validate_sector(pbn * PAGES_PER_BLOCK + td[tc] % PAGES_PER_BLOCK, (tc + 3) * 17))
		{
			printf("page 데이터 이상 없음.\n\n");
			score += 1;
		}
		else
		{
			printf("이상 발견. page 데이터의 값이 0x%d%d가 아닙니다. -1점\n\n", tc + 3, tc + 3);
		}

		printf("%d번 block의 %d번 page에 정상적으로 spare를 작성했는지 확인...\n", pbn, td[tc] % PAGES_PER_BLOCK);
		if (!validate_spare(pbn * PAGES_PER_BLOCK + td[tc] % PAGES_PER_BLOCK, td[tc]))
		{
			printf("page spare 이상 없음.\n\n");
			score += 2;
		}
		else
		{
			printf("-2점\n\n");
		}

		printf("다른 block에는 데이터를 쓰지 않았는지 확인...\n");
		if (!validate_empty_blocks(amt))
		{
			printf("다른 block 이상 없음.\n\n");
			score += 1;
		}
		else
		{
			printf("-1점\n\n");
		}
		printf("\n");
	}

	printf("ftl_write() - overwrite 점수 : %d/30\n\n", score);
	print_line();

	return score;
}

static int ftl_read_check_2() // 총 10점
{
	int score;
	int tc, td[3] = {PAGES_PER_BLOCK, PAGES_PER_BLOCK * 2 + 1, PAGES_PER_BLOCK * 2 + 2};
	int amt_org[DATABLKS_PER_DEVICE], pfb_org;
	int amt[DATABLKS_PER_DEVICE], pfb;
	char sectorbuf[SECTOR_SIZE];
	int flag;
	int i;

	score = 1;

	ftl_get_information(&pfb_org, amt_org);

	// ftl_write
	for (tc = 0; tc < 3; ++tc)
	{
		printf("ftl_read(%d, sectorbuf) 실행\n\n", td[tc]);
		ftl_read(td[tc], sectorbuf);
		ftl_get_information(&pfb, amt);

		printf("데이터를 온전히 읽었는지 확인...\n");
		if (!validate_data(sectorbuf, (tc + 3) * 17))
		{
			printf("데이터 이상 없음.\n\n", pfb);
			score += 1;
		}
		else
		{
			printf("이상 발견. 데이터의 값에 문제가 있습니다.\n");
			printf("sector[0]의 값이 0x%2X입니다. -1점\n\n", sectorbuf[0]);
		}

		printf("free block's pbn이 바뀌지 않았는지 확인...\n");
		if (pfb_org == pfb)
		{
			printf("free block's pbn 값 이상 없음.\n\n", pfb);
			score += 1;
		}
		else
		{
			printf("이상 발견. free block's pbn 값이 변경되었습니다.\n");
			printf("free block's pbn 값이 %d에서 %d로 변경되었습니다. -1점\n\n", pfb_org, pfb);
		}

		printf("address mapping table이 바뀌지 않았는지 확인...\n");
		flag = TRUE;
		for (i = 0; i < DATABLKS_PER_DEVICE; ++i)
		{
			if (amt_org[i] != amt[i])
			{
				flag = FALSE;
				break;
			}
		}
		if (flag)
		{
			printf("address mapping table 값 이상 없음.\n\n", pfb);
			score += 1;
		}
		else
		{
			printf("이상 발견. address mapping table이 변경되었습니다.\n");
			printf("table[%d] 값이 %d에서 %d로 변경되었습니다. -1점\n\n", i, amt_org[i], amt[i]);
		}
		printf("\n");
	}

	printf("ftl_read() - after overwrite 점수 : %d/10\n\n", score);
	print_line();

	return score;
}

static int ftl_open_check_2() // 총 10점
{
	int score;
	int amt[DATABLKS_PER_DEVICE], pfb;
	int amt_org[DATABLKS_PER_DEVICE];
	char sectorbuf[SECTOR_SIZE];
	char pagebuf[PAGE_SIZE];
	char buf[DATABLKS_PER_DEVICE * 10 + 30];
	int i, j, flag;

	// save
	printf("flashmemory 파일 닫기\n");
	ftl_get_information(&pfb, amt_org);
	fclose(flashfp);

	// initialize
	printf("address mapping table, free block's pbn 초기화\n");
	clear_flashmemory("flashmemory_empty");
	ftl_open();
	fclose(flashfp);

	// reopen
	printf("flashmemory 파일 다시 열기\n");
	printf("ftl_open() 실행\n");
	flashfp = fopen("flashmemory", "a+b");
	ftl_open();
	ftl_get_information(&pfb, amt);

	score = 0;

	printf("address mapping table이 바뀌지 않았는지 확인...\n");
	flag = TRUE;
	for (i = 0; i < DATABLKS_PER_DEVICE; ++i)
	{
		if (amt_org[i] != amt[i])
		{
			flag = FALSE;
			break;
		}
	}
	if (flag)
	{
		printf("address mapping table 값 이상 없음.\n\n", pfb);
		score += 5;
	}
	else
	{
		printf("이상 발견. address mapping table이 변경되었습니다.\n");
		printf("table[%d] 값이 %d에서 %d로 변경되었습니다. -5점\n\n", i, amt_org[i], amt[i]);
	}

	printf("free block's pbn이 실제로 비어있는지 확인...\n");
	flag = TRUE;
	for (i = 0; i < PAGES_PER_BLOCK; ++i)
	{
		if (validate_sector(pfb * PAGES_PER_BLOCK + i, 0xFF))
		{
			flag = FALSE;
			break;
		}
	}
	if (flag)
	{
		printf("free block 이상 없음.\n\n", pfb);
		score += 5;
	}
	else
	{
		printf("이상 발견. free block(%d번 block)이 비어 있지 않습니다.\n", pfb);
		printf("%d번 block의 %d번 page 값이 0xFF가 아닙니다. -5점\n\n", pfb, i);
	}

	printf("ftl_open() - after close 점수 : %d/10\n\n", score);
	print_line();

	return score;
}