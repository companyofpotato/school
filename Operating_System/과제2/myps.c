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
#define LEN 64
#define PID_CNT_MAX 32768
#define PROC_CNT_MAX 4096
#define DATA_CNT 24
#define DATA_LEN 32
#define CMD_LEN 1024
#define TTY_LEN 32
#define FN_LEN 128
#define COLUMN_CNT 12

#define PID_IDX 0
#define VSZ_IDX 1
#define RSS_IDX 2
#define CPU_IDX 0
#define MEM_IDX 1
#define USER_IDX 0
#define TTY_IDX 1
#define STAT_IDX 2
#define START_IDX 3
#define TIME_IDX 4
#define CMD_IDX 5
#define CMD2_IDX 6

#define OPT_a 0
#define OPT_u 1
#define OPT_x 2

typedef struct{
    int data_int[3];//pid, vsz, rss;
    double data_db[2];//cpu, mem
    char data_str[7][CMD_LEN];//user, tty, stat, start, time, cmd, cmd2(command)
}cusProc;

char buf[BUFFER_SIZE], mytty[TTY_LEN], col_name[COLUMN_CNT][10] = {"USER", "PID", "%CPU", "%MEM", "VSZ", "RSS", "TTY", "STAT", "START", "TIME", "CMD", "COMMAND"};
char col_data[PROC_CNT_MAX][COLUMN_CNT][LEN];
cusProc proc_list[PROC_CNT_MAX];
int proc_cnt, opt_mask, col_hash[COLUMN_CNT] = {1, 4, 5, 2, 3, 0, 6, 7, 8, 9, 10, 11};
int mypid, myuid;

long uptime, mem_total, cs_cnt, cpu_time[PID_CNT_MAX];
time_t bef, now;

long get_mem();
long get_uptime();
void proc_search();
void proc_add(char path[PATH_LEN]);
void proc_init(cusProc *proc);
void proc_erase();
void print_scr();
void get_tty(char path[PATH_LEN], char tty[TTY_LEN]);
void error(int ec);

int main(int argc, char **argv)
{
    int len;
    for(int i = 1;i < argc;i++)
    {
        len = strlen(argv[i]);
        int j = 0;
        for(;j < len;j++)
        {
            switch(argv[i][j])
            {
                case 'a': opt_mask |= 1 << OPT_a; break;
                case 'u': opt_mask |= 1 << OPT_u; break;
                case 'x': opt_mask |= 1 << OPT_x; break;
                default: error(0); break;
            }
        }
    }

    mem_total = get_mem();
    cs_cnt = sysconf(_SC_CLK_TCK);

    mypid = getpid();
    myuid = getuid();

    char path[FN_LEN];
    memset(path, 0, FN_LEN);
    sprintf(path, "/proc/%d", mypid);

    get_tty(path, mytty);
    len = strlen(mytty);
    for(int i = strlen("pts/");i < len;i++)
    {
        if(!isdigit(mytty[i]))
        {
            mytty[i] = 0;
            break;
        }
    }

    initscr();
    endwin();

    memset(cpu_time, 0, PID_CNT_MAX);
    proc_search();

    print_scr();

    return 0;
}

long get_mem()
{
    FILE *fp;
    fp = fopen("/proc/meminfo", "r");

    memset(buf, 0, BUFFER_SIZE);
    fgets(buf, BUFFER_SIZE, fp);

    int cur = 0;
    for(;cur < BUFFER_SIZE;cur++)
    {
        if(isdigit(buf[cur]))
            break;
    }

    long res;
    sscanf(buf + cur, "%ld", &res);

    fclose(fp);

    return res;
}

long get_uptime()
{
    FILE *fp;
    fp = fopen("/proc/uptime", "r");

    memset(buf, 0, BUFFER_SIZE);
    fgets(buf, BUFFER_SIZE, fp);

    long res;
    sscanf(buf, "%ld", &res);

    fclose(fp);

    return res;
}

void proc_search()
{
    uptime = get_uptime();
    DIR *dir;
    dir = opendir("/proc");

    struct dirent *de;
    char path[PATH_LEN];

    while((de = readdir(dir)) != NULL)
    {
        bool def = false;
        int len = strlen(de->d_name);
        for(int i = 0;i < len;i++)
        {
            if(!isdigit(de->d_name[i]))
            {
                def = true;
                break;
            } 
        }
        if(def)
            continue;

        memset(path, 0, PATH_LEN);
        sprintf(path, "/proc/%s", de->d_name);

        struct stat st;
        stat(path, &st);

        if(!S_ISDIR(st.st_mode))
            continue;

        if(!(opt_mask & 1 << OPT_a) && st.st_uid != myuid)
            continue;

        char tty[TTY_LEN];
        get_tty(path, tty);

        if(!(opt_mask & 1 << OPT_x))
        {
            if(strlen(tty) == 0 || strcmp(tty, "?") == 0)
                continue;
        }

        if(!(opt_mask & 1 << OPT_a) && !(opt_mask & 1 << OPT_u) && !(opt_mask & 1 << OPT_x))
        {
            if(strcmp(tty, mytty) != 0)
                continue;
        }

        proc_add(path);
    }
    closedir(dir);
}

void proc_add(char path[PATH_LEN])
{
    cusProc proc;
    proc_init(&proc);

    char st_path[PATH_LEN];
    memset(st_path, 0, PATH_LEN);
    sprintf(st_path, "%s/stat", path);

    FILE *fp;
    fp = fopen(st_path, "r");
    char stat_data[DATA_CNT][DATA_LEN];
    memset(stat_data, 0, DATA_CNT * DATA_LEN);
    for(int i = 0;i < DATA_CNT;i++)
    {
        fscanf(fp, "%s", stat_data[i]);
    }
    fclose(fp);

    proc.data_int[PID_IDX] = atoi(stat_data[0]);

    struct stat st;
    stat(st_path, &st);

    struct passwd *pwd = getpwuid(st.st_uid);

    char tmp[DATA_LEN];
    strcpy(tmp, pwd->pw_name);
    tmp[8] = 0;
    if(!strcmp(tmp, "systemd-"))
    {
        tmp[7] = '+';
        strcpy(proc.data_str[USER_IDX], tmp);
    }
    else
        strcpy(proc.data_str[USER_IDX], pwd->pw_name);

    double cpu;
    long long total = (long long)atoi(stat_data[13]) + (long long)atoi(stat_data[14]);
    int start_time = atoi(stat_data[21]);
    
    if(cpu_time[proc.data_int[PID_IDX]] != 0)
    {
        cpu = (total - cpu_time[proc.data_int[PID_IDX]]) / (double)(now - bef);
    }
    else
        cpu = total / (double)(uptime - (start_time / cs_cnt));
    cpu = cpu / cs_cnt * 100;
    cpu_time[proc.data_int[PID_IDX]] = total;

    if(isnan(cpu) || isinf(cpu) || cpu > 100 || cpu < 0)
        proc.data_db[CPU_IDX] = 0.0;
    else   
        proc.data_db[CPU_IDX] = cpu;

    char status_path[PATH_LEN];
    memset(status_path, 0, PATH_LEN);
    sprintf(status_path, "%s/status", path);

    fp = fopen(status_path, "r");

    long lck = 0;

    for(int i = 0;i < 18;i++)
    {
        memset(buf, 0, BUFFER_SIZE);
        fgets(buf, BUFFER_SIZE, fp);
    }

    char *cur;
    int len = strlen(buf);
    for(int i = 0;i < len;i++)
    {
        if(isdigit(buf[i]))
        {
            cur = buf + i;
            break;
        }
    }

    buf[6] = 0;
    if(strcmp(buf, "VmSize") != 0)
    {
        proc.data_int[VSZ_IDX] = 0;
        proc.data_int[RSS_IDX] = 0;
        proc.data_db[MEM_IDX] = 0.0;
    }
    else
    {
        sscanf(cur, "%d", &proc.data_int[VSZ_IDX]);

        memset(buf, 0, BUFFER_SIZE);
        fgets(buf, BUFFER_SIZE, fp);

        len = strlen(buf);
        for(int i = 0;i < len;i++)
        {
            if(isdigit(buf[i]))
            {
                cur = buf + i;
                break;
            }
        }
        sscanf(cur, "%ld", &lck);

        for(int i = 0;i < 3;i++)
        {
            memset(buf, 0, BUFFER_SIZE);
            fgets(buf, BUFFER_SIZE, fp);
        }
        len = strlen(buf);
        for(int i = 0;i < len;i++)
        {
            if(isdigit(buf[i]))
            {
                cur = buf + i;
                break;
            }
        }
        sscanf(cur, "%d", &proc.data_int[RSS_IDX]);

        double mem = (double)proc.data_int[RSS_IDX] / mem_total * 100;

        if(isnan(mem) || isinf(mem) || mem > 100 || mem < 0)
            proc.data_db[MEM_IDX] = 0.0;
        else
            proc.data_db[MEM_IDX] = mem;
    }
    fclose(fp);

    get_tty(path, proc.data_str[TTY_IDX]);

    strcpy(proc.data_str[STAT_IDX], stat_data[2]);

    int nice = atoi(stat_data[18]);
    if(nice > 0)
        strcat(proc.data_str[STAT_IDX], "N");
    else if(nice < 0)
        strcat(proc.data_str[STAT_IDX], "<");

    if(lck > 0)
        strcat(proc.data_str[STAT_IDX], "L");
    
    int session = atoi(stat_data[5]);
    if(session == proc.data_int[PID_IDX]);
        strcat(proc.data_str[STAT_IDX], "s");

    int thr_cnt = atoi(stat_data[19]);
    if(thr_cnt > 1)
        strcat(proc.data_str[STAT_IDX], "l");

    int fore_pid = atoi(stat_data[7]);
    if(fore_pid != -1)
        strcat(proc.data_str[STAT_IDX], "+");

    long start = time(NULL) - uptime + (start_time/cs_cnt);
    struct tm *tm_st = localtime(&start);
    if(time(NULL) - start < 24 * 60 * 60){
		strftime(proc.data_str[START_IDX], 16, "%H:%M", tm_st);
	}
	else if(time(NULL) - start < 7 * 24 * 60 * 60){
		strftime(proc.data_str[START_IDX], 16, "%b %d", tm_st);
	}
	else{
		strftime(proc.data_str[START_IDX], 16, "%y", tm_st);
	}

    int cputime = total / cs_cnt;
    if((opt_mask & 1 << OPT_a) || (opt_mask & 1 << OPT_u) || (opt_mask & 1 << OPT_x))
    {
        sprintf(proc.data_str[TIME_IDX], "%1d:%02d", (cputime % 3600) / 60, cputime % 60);
    }
    else
    {
        sprintf(proc.data_str[TIME_IDX], "%02d:%02d:%02d", cputime / 3600, (cputime % 3600) / 60, cputime % 60);
    }

    sscanf(stat_data[1], "(%s", proc.data_str[CMD_IDX]);
    proc.data_str[CMD_IDX][strlen(proc.data_str[CMD_IDX]) - 1] = 0;

    char cmd_path[PATH_LEN];
    memset(cmd_path, 0, PATH_LEN);
    sprintf(cmd_path, "%s/cmdline", path);

    fp = fopen(cmd_path, "r");

    while(1)
    {
        char c[2] = {0, 0};
        fread(&c[0], 1, 1, fp);
        if(c[0] == 0)
        {
            fread(&c[0], 1, 1, fp);
            if(c[0] == 0)
                break;
            else
                strcat(proc.data_str[CMD2_IDX], " ");
        }
        strcat(proc.data_str[CMD2_IDX], c);
    }

    if(strlen(proc.data_str[CMD2_IDX]) == 0)
    {
        strcat(proc.data_str[CMD2_IDX], "[");
        strcat(proc.data_str[CMD2_IDX], proc.data_str[CMD_IDX]);
        strcat(proc.data_str[CMD2_IDX], "]");
    }

    fclose(fp);

    proc_list[proc_cnt] = proc;
    proc_cnt++;
}

void proc_init(cusProc *proc)
{
    memset(proc->data_int, 0, 3);
    memset(proc->data_db, 0.0, 2);
    for(int i = 0;i < 7;i++)
    {
        memset(proc->data_str[i], 0, CMD_LEN);
    }
}

void proc_erase()
{
    for(int i = 0;i < proc_cnt;i++)
    {
        proc_init(&proc_list[i]);
    }
}

void print_scr()
{
    int col_width[COLUMN_CNT] = {4, 3, 4, 4, 3, 3, 3, 4, 5, 4, 3, 7};
    
    for(int i = 0;i < proc_cnt;i++)
    {
        int cur = 0;
        for(int j = 0;j < 3;j++)
        {
            memset(buf, 0, BUFFER_SIZE);
            sprintf(buf, "%d", proc_list[i].data_int[j]);
            strcpy(col_data[i][col_hash[cur]], buf);
            if(col_width[col_hash[cur]] < strlen(buf))
                col_width[col_hash[cur]] = strlen(buf);
            cur++;
        }

        for(int j = 0;j < 2;j++)
        {
            memset(buf, 0, BUFFER_SIZE);
            sprintf(buf, "%2.1lf", proc_list[i].data_db[j]);
            strcpy(col_data[i][col_hash[cur]], buf);
            if(col_width[col_hash[cur]] < strlen(buf))
                col_width[col_hash[cur]] = strlen(buf);
            cur++;
        }

        for(int j = 0;j < 7;j++)
        {
            strcpy(col_data[i][col_hash[cur]], proc_list[i].data_str[j]);
            if(col_width[col_hash[cur]] < strlen(proc_list[i].data_str[j]))
                col_width[col_hash[cur]] = strlen(proc_list[i].data_str[j]);
            cur++;
        }
    }

    int col_cnt;
    int *col_list, *col_gap;

    if(opt_mask & 1 << OPT_u)
    {
        int tmp_list[11] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 11};
        int tmp_gap[11] = {5, 1, 1, 2, 2, 1, 4, 1, 3, 1, 0};
        col_list = tmp_list;
        col_gap = tmp_gap;
        col_cnt = 11;
    }
    else if((opt_mask & 1 << OPT_a) || (opt_mask & 1 << OPT_x))
    {
        int tmp_list[5] = {1, 6, 7, 9, 11};
        int tmp_gap[5] = {1, 4, 3, 1, 0};
        col_list = tmp_list;
        col_gap = tmp_gap;
        col_cnt = 5;
    }
    else
    {
        int tmp_list[4] = {1, 6, 9, 10};
        int tmp_gap[4] = {1, 4, 1, 0};
        col_list = tmp_list;
        col_gap = tmp_gap;
        col_cnt = 4;
    }

    int gap;
    char buf_print[BUFFER_SIZE];
    memset(buf_print, 0, BUFFER_SIZE);
    
    for(int i = 0;i < col_cnt;i++)
    {
        gap = col_width[col_list[i]] - strlen(col_name[col_list[i]]);
        
        if(col_list[i] == 1)
            strcat(buf_print, "   ");

        if(col_list[i] == 0 || col_list[i] == 6 || col_list[i] == 7 || col_list[i] == 8)
        {
            strcat(buf_print, col_name[col_list[i]]);
            for(int j = 0;j < gap;j++)
            {
                strcat(buf_print, " ");
            }
        }
        else if(col_list[i] == 10 || col_list[i] == 11)
        {
            strcat(buf_print, col_name[col_list[i]]);
        }
        else
        {
            for(int j = 0;j < gap;j++)
            {
                strcat(buf_print, " ");
            }
            strcat(buf_print, col_name[col_list[i]]);
        }

        for(int j = 0;j < col_gap[i];j++)
        {
            strcat(buf_print, " ");
        }
    }
    buf_print[COLS] = 0;
    printf("%s\n", buf_print);

    for(int p = 0;p < proc_cnt;p++)
    {
        memset(buf_print, 0, BUFFER_SIZE);
        for(int i = 0;i < col_cnt;i++)
        {
            gap = col_width[col_list[i]] - strlen(col_data[p][col_list[i]]);
            
            if(col_list[i] == 1)
                strcat(buf_print, "   ");

            if(col_list[i] == 0 || col_list[i] == 6 || col_list[i] == 7 || col_list[i] == 8)
            {
                strcat(buf_print, col_data[p][col_list[i]]);
                for(int j = 0;j < gap;j++)
                {
                    strcat(buf_print, " ");
                }
            }
            else if(col_list[i] == 10 || col_list[i] == 11)
            {
                strcat(buf_print, col_data[p][col_list[i]]);
            }
            else
            {
                for(int j = 0;j < gap;j++)
                {
                    strcat(buf_print, " ");
                }
                strcat(buf_print, col_data[p][col_list[i]]);
            }

            for(int j = 0;j < col_gap[i];j++)
            {
                strcat(buf_print, " ");
            }
        }
        buf_print[COLS] = 0;
        printf("%s\n", buf_print);
    }
}

void get_tty(char path[PATH_LEN], char tty[TTY_LEN])
{
    memset(tty, 0, TTY_LEN);
    char fd_path[PATH_LEN];
    memset(fd_path, 0, PATH_LEN);
    sprintf(fd_path, "%s/fd/0", path);

    if(access(fd_path, F_OK) < 0)
    {
        char stat_path[PATH_LEN];
        memset(stat_path, 0, PATH_LEN);
        sprintf(stat_path, "%s/stat", path);

        FILE *fp;
        fp = fopen(stat_path, "r");

        for(int i = 0;i < 7;i++)
        {
            memset(buf, 0, BUFFER_SIZE);
            fscanf(fp, "%s", buf);
        }

        int tty_num = atoi(buf);

        fclose(fp);

        DIR *dir;
        dir = opendir("/dev");

        char buf_path[PATH_LEN];

        struct dirent *de;
        while((de = readdir(dir)) != NULL)
        {
            memset(buf_path, 0, PATH_LEN);
            sprintf(buf_path, "/dev/%s", de->d_name);

            struct stat st;
            stat(buf_path, &st);

            if(!S_ISCHR(st.st_mode))
                continue;
            else if(st.st_rdev == tty_num)
            {
                strcpy(tty, de->d_name);
                break;
            }
        }

        closedir(dir);

        if(strlen(tty) == 0)
            strcpy(tty, "?");
    }
    else
    {
        char symlink[FN_LEN];
        memset(symlink, 0, FN_LEN);
        readlink(fd_path, symlink, FN_LEN);

        if(strcmp(symlink, "/dev/null") == 0)
            strcpy(tty, "?");
        else
            sscanf(symlink, "/dev/%s", tty);
    }
}

void error(int ec)
{
    switch(ec)
    {
        case 0: printf("error\n"); break;
    }
    exit(1);
}