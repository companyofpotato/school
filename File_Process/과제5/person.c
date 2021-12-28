#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "person.h"
#include "time.h"

#define ID_SIZE 14
#define NAME_SIZE 18
#define AGE_SIZE 4
#define ADDR_SIZE 22
#define PHONE_SIZE 16
#define EMAIL_SIZE 26
#define HEADER_SIZE 16
#define CHAR_SIZE sizeof(char)
#define INT_SIZE sizeof(int)

int readcnt = 0;

void readPage(FILE *fp, char *pagebuf, int pagenum);
void writePage(FILE *fp, const char *pagebuf, int pagenum); 
void pack(char *recordbuf, const Person *p);
void unpack(const char *recordbuf, Person *p);
void add(FILE *fp, const Person *p);
void delete(FILE *fp, const char *id);
void readHeader(FILE *fp, int *pcnt, int *rcnt, int *pnum, int *rnum);
void writeHeader(FILE *fp, int *pcnt, int *rcnt, int *pnum, int *rnum);
void cus_sort(char **data, int cnt);
void createIndex(FILE *idxfp, FILE *recordfp);
void binarysearch(FILE *idxfp, const char *id, int *pageNum, int *recordNum);
void cus_error(int);

int main(int argc, char *argv[])
{
	FILE *fp, *idxfp;

	if(argc < 4)
		cus_error(0);

	if(strlen(argv[1]) != 1)
		cus_error(1);

	fp = fopen(argv[2], "r+b");
	if(fp == NULL)
	{
		fp = fopen(argv[2], "w+b");
		int pcnt = 0, rcnt = 0, pnum = -1, rnum = -1;
		writeHeader(fp, &pcnt, &rcnt, &pnum, &rnum);
	}

	if(argv[1][0] == 'a')
	{
		Person p;
		char id[ID_SIZE] = "";
		char name[NAME_SIZE] = "";
		char age[AGE_SIZE] = "";
		char addr[ADDR_SIZE] = "";
		char phone[PHONE_SIZE] = "";
		char email[EMAIL_SIZE] = "";
		memcpy(id, argv[3], CHAR_SIZE * strlen(argv[3]));
		memcpy(name, argv[4], CHAR_SIZE * strlen(argv[4]));
		memcpy(age, argv[5], CHAR_SIZE * strlen(argv[5]));
		memcpy(addr, argv[6], CHAR_SIZE * strlen(argv[6]));
		memcpy(phone, argv[7], CHAR_SIZE * strlen(argv[7]));
		memcpy(email, argv[8], CHAR_SIZE * strlen(argv[8]));
		
		memcpy(p.id, id, CHAR_SIZE * ID_SIZE);
		memcpy(p.name, name, CHAR_SIZE * NAME_SIZE);
		memcpy(p.age, age, CHAR_SIZE * AGE_SIZE);
		memcpy(p.addr, addr, CHAR_SIZE * ADDR_SIZE);
		memcpy(p.phone, phone, CHAR_SIZE * PHONE_SIZE);
		memcpy(p.email, email, CHAR_SIZE * EMAIL_SIZE);

		add(fp, &p);
	}
	else if(argv[1][0] == 'd')
	{
		char id[ID_SIZE] = "";
		memcpy(id, argv[3], CHAR_SIZE * strlen(argv[3]));
		delete(fp, id);
	}
	else if(argv[1][0] == 'i')
	{
		idxfp = fopen(argv[3], "w+b");
		createIndex(idxfp, fp);
	}
	else if(argv[1][0] == 'b')
	{
		idxfp = fopen(argv[3], "r+b");
		char id[ID_SIZE] = "";
		memcpy(id, argv[4], CHAR_SIZE * strlen(argv[4]));
		int *pnum = malloc(INT_SIZE), *rnum = malloc(INT_SIZE);
		binarysearch(idxfp, id, pnum, rnum);
		
		printf("#reads:%d\n", readcnt);
		if(*pnum == -1 && *rnum == -1)
			printf("no persons\n");
		else
		{
			char pagebuf[PAGE_SIZE] = "";
			readPage(fp, pagebuf, *pnum);

			char harea[HEADER_AREA_SIZE] = "", darea[DATA_AREA_SIZE] = "";
			memcpy(harea, pagebuf, CHAR_SIZE * HEADER_AREA_SIZE);
			memcpy(darea, pagebuf + HEADER_AREA_SIZE, CHAR_SIZE * DATA_AREA_SIZE);

			int offset, len;
			memcpy(&offset, harea + INT_SIZE * (2 * (*rnum) + 1), INT_SIZE);
			memcpy(&len, harea + INT_SIZE * (2 * (*rnum) + 2), INT_SIZE);
			
			char recordbuf[MAX_RECORD_SIZE] = "";
			memcpy(recordbuf, darea + offset, CHAR_SIZE * len);

			Person p;
			unpack(recordbuf, &p);

			printf("id=%s\n", p.id);
			printf("name=%s\n", p.name);
			printf("age=%s\n", p.age);
			printf("addr=%s\n", p.addr);
			printf("phone=%s\n", p.phone);
			printf("email=%s\n", p.email);
		}
	}
	else
	{
		cus_error(2);
	}

	fclose(fp);
	return 0;
}

void readPage(FILE *fp, char *pagebuf, int pagenum)
{
	fseek(fp, PAGE_SIZE * pagenum + HEADER_SIZE, SEEK_SET);
	fread((void *)pagebuf, PAGE_SIZE, 1, fp);
}

void writePage(FILE *fp, const char *pagebuf, int pagenum)
{
	fseek(fp, PAGE_SIZE * pagenum + HEADER_SIZE, SEEK_SET);
	fwrite((void *)pagebuf, PAGE_SIZE, 1, fp);
}
 
void pack(char *recordbuf, const Person *p)
{
	char* cursor = recordbuf;

	memcpy(cursor, p->id, CHAR_SIZE * strlen(p->id));
	cursor += strlen(p->id);
	*cursor = '#';
	cursor++;
	memcpy(cursor, p->name, CHAR_SIZE * strlen(p->name));
	cursor += strlen(p->name);
	*cursor = '#';
	cursor++;
	memcpy(cursor, p->age, CHAR_SIZE * strlen(p->age));
	cursor += strlen(p->age);
	*cursor = '#';
	cursor++;
	memcpy(cursor, p->addr, CHAR_SIZE * strlen(p->addr));
	cursor += strlen(p->addr);
	*cursor = '#';
	cursor++;
	memcpy(cursor, p->phone, CHAR_SIZE * strlen(p->phone));
	cursor += strlen(p->phone);
	*cursor = '#';
	cursor++;
	memcpy(cursor, p->email, CHAR_SIZE * strlen(p->email));
	cursor += strlen(p->email);
	*cursor = '#';
}

void unpack(const char *recordbuf, Person *p)
{
	char* tmpbuf = malloc(CHAR_SIZE * strlen(recordbuf));

	memcpy(tmpbuf, recordbuf, CHAR_SIZE * strlen(recordbuf));

	char id[ID_SIZE] = "";
	char name[NAME_SIZE] = "";
	char age[AGE_SIZE] = "";
	char addr[ADDR_SIZE] = "";
	char phone[PHONE_SIZE] = "";
	char email[EMAIL_SIZE] = "";

	char *ptr = strtok(tmpbuf, "#");
	
	memcpy(id, ptr, CHAR_SIZE * ID_SIZE);
	ptr = strtok(NULL, "#");
	memcpy(name, ptr, CHAR_SIZE * NAME_SIZE);
	ptr = strtok(NULL, "#");
	memcpy(age, ptr, CHAR_SIZE * AGE_SIZE);
	ptr = strtok(NULL, "#");
	memcpy(addr, ptr, CHAR_SIZE * ADDR_SIZE);
	ptr = strtok(NULL, "#");
	memcpy(phone, ptr, CHAR_SIZE * PHONE_SIZE);
	ptr = strtok(NULL, "#");
	memcpy(email, ptr, CHAR_SIZE * EMAIL_SIZE);

	memcpy(p->id, id, CHAR_SIZE * ID_SIZE);
	memcpy(p->name, name, CHAR_SIZE * NAME_SIZE);
	memcpy(p->age, age, CHAR_SIZE * AGE_SIZE);
	memcpy(p->addr, addr, CHAR_SIZE * ADDR_SIZE);
	memcpy(p->phone, phone, CHAR_SIZE * PHONE_SIZE);
	memcpy(p->email, email, CHAR_SIZE * EMAIL_SIZE);
}

void add(FILE *fp, const Person *p)
{
	int pcnt, rcnt, pnum, rnum;
	readHeader(fp, &pcnt, &rcnt, &pnum, &rnum);

	char recordbuf[MAX_RECORD_SIZE] = "";
	pack(recordbuf, p);

	int rlen = strlen(recordbuf);

	int tmpp = pnum, tmpr = rnum, befp = -1, befr = -1;
	while(tmpp != -1 && tmpr != -1)
	{
		char pagebuf[PAGE_SIZE] = "";
		readPage(fp, pagebuf, tmpp);

		char harea[HEADER_AREA_SIZE] = "", darea[DATA_AREA_SIZE] = "";
		memcpy(harea, pagebuf, CHAR_SIZE * HEADER_AREA_SIZE);
		memcpy(darea, pagebuf + HEADER_AREA_SIZE, CHAR_SIZE * DATA_AREA_SIZE);

		int emptyspace = 0, offset = 0, cursor = INT_SIZE * (2 * tmpr + 1);
		memcpy(&offset, harea + cursor, INT_SIZE);
		memcpy(&emptyspace, harea + cursor + INT_SIZE, INT_SIZE);

		char tmpbuf[MAX_RECORD_SIZE] = "";
		memcpy(tmpbuf, darea + offset, CHAR_SIZE * emptyspace);

		if(emptyspace >= rlen)
		{
			int orip = tmpp;
			if(befp == -1 && befr == -1)
			{
				memcpy(&pnum, tmpbuf + CHAR_SIZE, INT_SIZE);
				memcpy(&rnum, tmpbuf + CHAR_SIZE + INT_SIZE, INT_SIZE);
			}
			else
			{
				memcpy(&tmpp, tmpbuf + CHAR_SIZE, INT_SIZE);
				memcpy(&tmpr, tmpbuf + CHAR_SIZE + INT_SIZE, INT_SIZE);

				char befpage[PAGE_SIZE] = "";
				readPage(fp, befpage, befp);

				char befh[HEADER_AREA_SIZE] = "", befd[DATA_AREA_SIZE] = "";
				memcpy(befh, befpage, CHAR_SIZE * HEADER_AREA_SIZE);
				memcpy(befd, befpage + HEADER_AREA_SIZE, CHAR_SIZE * DATA_AREA_SIZE);

				int befoff = 0, beflen = 0, befcur = INT_SIZE * (2 * befr + 1);
				memcpy(&befoff, befh + befcur, INT_SIZE);
				memcpy(&beflen, befh + befcur + INT_SIZE, INT_SIZE);
				
				char befrec[MAX_RECORD_SIZE] = "";
				memcpy(befrec, befd + befoff, CHAR_SIZE * beflen);

				memcpy(befrec + CHAR_SIZE, &tmpp, INT_SIZE);
				memcpy(befrec + CHAR_SIZE + INT_SIZE, &tmpr, INT_SIZE);

				memcpy(befd + befoff, befrec, CHAR_SIZE * beflen);
				memcpy(befpage + HEADER_AREA_SIZE, befd, CHAR_SIZE * DATA_AREA_SIZE);
				writePage(fp, befpage, befp);
			}

			memcpy(darea + offset, recordbuf, CHAR_SIZE * emptyspace);
			memcpy(pagebuf + HEADER_AREA_SIZE, darea, CHAR_SIZE * DATA_AREA_SIZE);
			writePage(fp, pagebuf, orip);
			writeHeader(fp, &pcnt, &rcnt, &pnum, &rnum);
			return;
		}
		else
		{
			befp = tmpp;
			befr = tmpr;
			memcpy(&tmpp, tmpbuf + CHAR_SIZE, INT_SIZE);
			memcpy(&tmpr, tmpbuf + CHAR_SIZE + INT_SIZE, INT_SIZE);
		}
	}

	if(pcnt > 0)
	{
		char pagebuf[PAGE_SIZE] = "";
		readPage(fp, pagebuf, pcnt - 1);
		
		char harea[HEADER_AREA_SIZE] = "", darea[DATA_AREA_SIZE] = "";
		memcpy(harea, pagebuf, CHAR_SIZE * HEADER_AREA_SIZE);
		memcpy(darea, pagebuf + HEADER_AREA_SIZE, CHAR_SIZE * DATA_AREA_SIZE);

		int cnt = 0, sum = 0, len = 0, cursor = INT_SIZE;
		memcpy(&cnt, harea, INT_SIZE);
		for(int i = 0;i < cnt;i++)
		{
			cursor += INT_SIZE;
			len = 0;
			memcpy(&len, harea + cursor, INT_SIZE);
			cursor += INT_SIZE;
			sum += len;
		}
		
		if((sum + rlen) <= DATA_AREA_SIZE && (cursor + 2 * INT_SIZE) <= HEADER_AREA_SIZE)
		{
			int offset = sum;
			memcpy(darea + offset, recordbuf, CHAR_SIZE * rlen);

			cnt++;
			memcpy(harea, &cnt, INT_SIZE);
			memcpy(harea + cursor, &offset, INT_SIZE);
			cursor += INT_SIZE;
			memcpy(harea + cursor, &rlen, INT_SIZE);

			memcpy(pagebuf, harea, CHAR_SIZE * HEADER_AREA_SIZE);
			memcpy(pagebuf + HEADER_AREA_SIZE, darea, CHAR_SIZE * DATA_AREA_SIZE);
			writePage(fp, pagebuf, pcnt - 1);

			rcnt++;
			writeHeader(fp, &pcnt, &rcnt, &pnum, &rnum);
			return;
		}
	}

	char newpagebuf[PAGE_SIZE] = "";

	char newharea[HEADER_AREA_SIZE] = "";
	int newcnt = 1, newcursor = INT_SIZE;
	memcpy(newharea, &newcnt, INT_SIZE);
	newcnt--;
	memcpy(newharea + newcursor, &newcnt, INT_SIZE);
	newcursor += INT_SIZE;
	memcpy(newharea + newcursor, &rlen, INT_SIZE);
	memcpy(newpagebuf, newharea, CHAR_SIZE * HEADER_AREA_SIZE);

	char newdarea[DATA_AREA_SIZE] = "";
	memcpy(newdarea, recordbuf, CHAR_SIZE * rlen);
	memcpy(newpagebuf + HEADER_AREA_SIZE, newdarea, CHAR_SIZE * DATA_AREA_SIZE);
	writePage(fp, newpagebuf, pcnt);

	pcnt++;
	rcnt++;
	writeHeader(fp, &pcnt, &rcnt, &pnum, &rnum);
}

void delete(FILE *fp, const char *id)
{
	int pcnt, rcnt, pnum, rnum;
	readHeader(fp, &pcnt, &rcnt, &pnum, &rnum);

	char pagebuf[PAGE_SIZE], empbuf[PAGE_SIZE] = "";
	for(int i = 0;i < pcnt;i++)
	{
		memcpy(pagebuf, empbuf, CHAR_SIZE * PAGE_SIZE);
		readPage(fp, pagebuf, i);

		char harea[HEADER_AREA_SIZE] = "", darea[DATA_AREA_SIZE] = "";
		memcpy(harea, pagebuf, CHAR_SIZE * HEADER_AREA_SIZE);
		memcpy(darea, pagebuf + HEADER_AREA_SIZE, CHAR_SIZE * DATA_AREA_SIZE);

		int cnt, offset, len, cursor = INT_SIZE;
		memcpy(&cnt, harea, INT_SIZE);
		for(int j = 0;j < cnt;j++)
		{
			offset = 0;
			memcpy(&offset, harea + cursor, INT_SIZE);
			cursor += INT_SIZE;

			len = 0;
			memcpy(&len, harea + cursor, INT_SIZE);
			cursor += INT_SIZE;

			char recordbuf[MAX_RECORD_SIZE] = "";
			memcpy(recordbuf, darea + offset, CHAR_SIZE * len);

			Person p;
			unpack(recordbuf, &p);

			if(strcmp(id, p.id) == 0)
			{
				recordbuf[0] = '*';
				memcpy(recordbuf + CHAR_SIZE, &pnum, INT_SIZE);
				memcpy(recordbuf + CHAR_SIZE + INT_SIZE, &rnum, INT_SIZE);

				pnum = i;
				rnum = j;
				
				memcpy(darea + offset, recordbuf, CHAR_SIZE * len);
				memcpy(pagebuf + HEADER_AREA_SIZE, darea, DATA_AREA_SIZE);
				writePage(fp, pagebuf, i);
				writeHeader(fp, &pcnt, &rcnt, &pnum, &rnum);
				return;
			}
		}
	}
	cus_error(3);
}

void readHeader(FILE *fp, int *pcnt, int *rcnt, int *pnum, int *rnum)
{
	fseek(fp, 0, SEEK_SET);
	fread(pcnt, INT_SIZE, 1, fp);
	fread(rcnt, INT_SIZE, 1, fp);
	fread(pnum, INT_SIZE, 1, fp);
	fread(rnum, INT_SIZE, 1, fp);
}

void writeHeader(FILE *fp, int *pcnt, int *rcnt, int *pnum, int *rnum)
{
	fseek(fp, 0, SEEK_SET);
	fwrite(pcnt, INT_SIZE, 1, fp);
	fwrite(rcnt, INT_SIZE, 1, fp);
	fwrite(pnum, INT_SIZE, 1, fp);
	fwrite(rnum, INT_SIZE, 1, fp);
}

void cus_sort(char **data, int cnt)
{
	int def = 1;
	while(def)
	{
		def = 0;
		for(int i = 0;i < cnt - 1;i++)
		{
			char t1[14] = "", t2[14] = "";
			memcpy(t1, data[i], CHAR_SIZE * 13);
			memcpy(t2, data[i + 1], CHAR_SIZE * 13);
			long n1 = strtol(t1, NULL, 10), n2 = strtol(t2, NULL, 10);
			if(n1 > n2)
			{
				char *tmp = data[i];
				data[i] = data[i + 1];
				data[i + 1] = tmp;
				def = 1;
			}
		}
	}
}

void createIndex(FILE *idxfp, FILE *recordfp)
{
	int pcnt, rcnt, pnum, rnum;
	readHeader(recordfp, &pcnt, &rcnt, &pnum, &rnum);

	char **idxdata = malloc(sizeof(char *) * rcnt);
	int rrcnt = 0;

	char pagebuf[PAGE_SIZE], empbuf[PAGE_SIZE] = "";
	for(int i = 0;i < pcnt;i++)
	{
		memcpy(pagebuf, empbuf, CHAR_SIZE * PAGE_SIZE);
		readPage(recordfp, pagebuf, i);

		char harea[HEADER_AREA_SIZE] = "", darea[DATA_AREA_SIZE] = "";
		memcpy(harea, pagebuf, CHAR_SIZE * HEADER_AREA_SIZE);
		memcpy(darea, pagebuf + HEADER_AREA_SIZE, CHAR_SIZE * DATA_AREA_SIZE);

		int cnt, offset, len, cursor = INT_SIZE;
		memcpy(&cnt, harea, INT_SIZE);
		for(int j = 0;j < cnt;j++)
		{
			offset = 0;
			memcpy(&offset, harea + cursor, INT_SIZE);
			cursor += INT_SIZE;

			len = 0;
			memcpy(&len, harea + cursor, INT_SIZE);
			cursor += INT_SIZE;

			char recordbuf[MAX_RECORD_SIZE] = "";
			memcpy(recordbuf, darea + offset, CHAR_SIZE * len);

			Person p;
			unpack(recordbuf, &p);

			if(p.id[0] != '*')
			{
				idxdata[rrcnt] = malloc(CHAR_SIZE * 21);
				memcpy(idxdata[rrcnt], p.id, CHAR_SIZE * 13);
				memcpy(idxdata[rrcnt] + CHAR_SIZE * 13, &i, INT_SIZE);
				memcpy(idxdata[rrcnt] + CHAR_SIZE * 13 + INT_SIZE, &j, INT_SIZE);
				rrcnt++;
			}
		}
	}
	
	cus_sort(idxdata, rrcnt);

	fseek(idxfp, 0, SEEK_SET);

	fwrite(&rrcnt, INT_SIZE, 1, idxfp);

	for(int i = 0;i < rrcnt;i++)
	{
		fwrite(idxdata[i], CHAR_SIZE * 21, 1, idxfp);
	}
}

void binarysearch(FILE *idxfp, const char *id, int *pageNum, int *recordNum)
{
	readcnt = 0;
	int rrcnt, def;
	fseek(idxfp, 0, SEEK_SET);
	fread(&rrcnt, INT_SIZE, 1, idxfp);
	char curid[14] = "";
	long ori = strtol(id, NULL, 10);
	long st = 0, end = rrcnt - 1, mid;

	while(st <= end)
	{
		mid = (st + end) / 2;
		fseek(idxfp, 4 + mid * 21, SEEK_SET);
		fread(curid, CHAR_SIZE * 13, 1, idxfp);
		fseek(idxfp, -13, SEEK_CUR);
		long curn = strtol(curid, NULL, 10);
		readcnt++;
		if(ori > curn)
		{
			st = mid + 1;
		}
		else if(ori < curn)
		{
			end = mid - 1;
		}
		else
		{
			fseek(idxfp, 13, SEEK_CUR);
			fread(pageNum, INT_SIZE, 1, idxfp);
			fread(recordNum, INT_SIZE, 1, idxfp);
			return;
		}
	}

	*(pageNum) = -1;
	*(recordNum) = -1;
}

void cus_error(int n)
{
	switch(n)
	{
		case 0: printf("Not enough argument!\n\n"); break;
		case 1: printf("Short argument!\n\n"); break;
		case 2: printf("Wrong argument!\n\n"); break;
		case 3: printf("No file!\n\n"); break;
	}
	exit(0);
}