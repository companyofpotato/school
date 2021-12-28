#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <limits.h>
#include <ctype.h>
#include <dirent.h>
#include <pwd.h>
#include <utmp.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <curses.h>

#define BUFFER_SIZE 1024
#define PATH_LEN 512
#define LEN 256
#define ROW_CNT 34
#define ROW_LEN 33

char buf[BUFFER_SIZE];

char row_name[ROW_CNT][ROW_LEN] = {
    "Architecture:",
    "CPU op-mode(s):",
    "Byte Order:",
    "Address sizes:",
    "CPU(s):",
    "On-line CPU(s) list:",
    "Thread(s) per core:",
    "Core(s) per socket:",
    "Socket(s):",
    "NUMA node(s):",
    "Vendor ID:",
    "CPU family:",
    "Model:",
    "Model name:",
    "Stepping:",
    "CPU MHz:",
    "BogoMIPS:",
    "Hypervisor vendor:",
    "Virtualization type:",
    "L1d cache:",
    "L1i cache:",
    "L2 cache:",
    "L3 cache:",
    "NUMA node0 CPU(s):",
    "Vulnerability Itlb multihit:",
    "Vulnerability L1tf:",
    "Vulnerability Mds:",
    "Vulnerability Meltdown:",
    "Vulnerability Spec store bypass:",
    "Vulnerability Spectre v1:",
    "Vulnerability Spectre v2:",
    "Vulnerability Srbds:",
    "Vulnerability Tsx async abort:",
    "Flags:"
};

char vul_name[9][30] = {
    "itlb_multihit",
    "l1tf",
    "mds",
    "meltdown",
    "spec_store_bypass",
    "spectre_v1",
    "spectre_v2",
    "srbds",
    "tsx_async_abort"
};

char *row_data[ROW_CNT];

int hash_row[11] = {10, 11, 12, 13, 14, 15, 8, 7, 33, 16, 3};
int hash_read[11] = {2, 1, 1, 1, 1, 2, 3, 2, 7, 2, 3};

void find_data();
void print_scr();

int main()
{
    initscr();
    endwin();

    find_data();
    print_scr();

    return 0;
}

void find_data()
{
    for(int i = 0;i < ROW_CNT - 1;i++)
    {
        row_data[i] = calloc(LEN, sizeof(char));
    }
    row_data[ROW_CNT - 1] = calloc(BUFFER_SIZE, sizeof(char));

    FILE *fp;

    fp = fopen("/sys/devices/system/cpu/online", "r");

    memset(buf, 0, BUFFER_SIZE);
    fgets(buf, BUFFER_SIZE, fp);

    int cpu_cnt = atoi(buf + 2) + 1;
    int cur;
    int sib = -1;
    int hyper = 1;

     //CPU(s)
    sprintf(row_data[4], "%d", cpu_cnt);

    //On-line CPU(s) list
    char tmp[LEN];
    for(int i = 0;i < cpu_cnt;i++) 
    {
        memset(tmp, 0, LEN);
        sprintf(tmp, "%d,", i);
        if(i == cpu_cnt - 1)
            tmp[strlen(tmp) - 1] = 0;
        strcat(row_data[5], tmp);
    }

    fclose(fp);

    fp = fopen("/proc/cpuinfo", "r");

    for(int i = 0;i < 11;i++)
    {
        for(int j = 0;j < hash_read[i];j++)
        {
            memset(buf, 0, BUFFER_SIZE);
            fgets(buf, BUFFER_SIZE, fp);
        }
        buf[strlen(buf) - 1] = 0;
        cur = 0;
        while(buf[cur] != ':')
            cur++;
        cur++;
        while(buf[cur] == ' ')
            cur++;
        if(hash_row[i] == 8)
        {
            sib = atoi(buf + cur);
            if(sib == cpu_cnt * 2)
            {
                hyper = 2;
            }
            continue;
        }
        strcpy(row_data[hash_row[i]], buf + cur);
    }

    fclose(fp);

    sprintf(row_data[8], "%d", cpu_cnt / (atoi(row_data[7]) * hyper));
    sprintf(row_data[6], "%d", cpu_cnt / (atoi(row_data[7]) * atoi(row_data[8])));

    char path[PATH_LEN], prefix[PATH_LEN - 128];
    memset(prefix, 0, PATH_LEN - 128);
    strcpy(prefix, "/sys/devices/system/cpu/vulnerabilities/");

    for(int i = 0;i < 9;i++)
    {
        memset(path, 0, PATH_LEN);
        sprintf(path, "%s%s", prefix, vul_name[i]);
        fp = fopen(path, "r");
        memset(buf, 0, BUFFER_SIZE);
        fgets(buf, BUFFER_SIZE, fp);
        buf[strlen(buf) - 1] = 0;
        strcpy(row_data[24 + i], buf);
        fclose(fp);
    }

    memset(prefix, 0, PATH_LEN - 128);
    strcpy(prefix, "/sys/devices/system/cpu/cpu");

    int cache_size[4] = {0, };
    for(int i = 0;i < cpu_cnt;i++)
    {
        for(int j = 0;j < 4;j++)
        {
            memset(path, 0, PATH_LEN);
            sprintf(path, "%s%d%s%d%s", prefix, i, "/cache/index", j, "/size");
            fp = fopen(path, "r");
            memset(buf, 0, BUFFER_SIZE);
            fgets(buf, BUFFER_SIZE, fp);
            buf[strlen(buf) - 2] = 0;
            cache_size[j] += atoi(buf);
            fclose(fp);
        }
    }

    for(int i = 0;i < 2;i++)
    {
        sprintf(row_data[19 + i], "%d KiB", cache_size[i]);
    }
    for(int i = 2;i < 4;i++)
    {
        sprintf(row_data[19 + i], "%.1f MiB", (float)cache_size[i] / 1000);
    }
}

void print_scr()
{
    int data_len = COLS - ROW_LEN, line;
    char blank[ROW_LEN], gap[ROW_LEN];
    memset(blank, ' ', ROW_LEN);

    for(int i = 0;i < ROW_CNT;i++)
    {
        memset(gap, 0, ROW_LEN);
        memset(gap, ' ', ROW_LEN - strlen(row_name[i]));
        line = strlen(row_data[i]) / data_len;
        if(line > 0 && (strlen(row_data[i]) % data_len) != 0)
            line++;
        if(line == 0)
            printf("%s%s%s\n", row_name[i], gap, row_data[i]);
        else
        {
            for(int j = 0;j < line;j++)
            {
                if(j == 0)
                    printf("%s%s", row_name[i], gap);
                else
                    printf("%s", blank);
                for(int k = 0;k < data_len;k++)
                {
                    printf("%c", row_data[i][j * data_len + k]);
                }
                printf("\n");
            }
        }
    }
}