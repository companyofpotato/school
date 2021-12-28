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
#define COLUMN_CNT 12
#define TICK_CNT 8

#define PID_IDX 0
#define PR_IDX 1
#define NI_IDX 2
#define VIRT_IDX 3
#define RES_IDX 4
#define SHR_IDX 5
#define CPU_IDX 0
#define MEM_IDX 1
#define USER_IDX 0
#define STATE_IDX 1
#define TIME_IDX 2
#define CMD_IDX 3

#define OPT_d 0
#define OPT_s 0
#define OPT_u 1
#define OPT_p 2
#define OPT_B 3
#define OPT_b 4
#define OPT_x 5
#define OPT_y 6
#define OPT_l 7
#define OPT_R 8

#define SORT_M 0
#define SORT_N 1
#define SORT_T 2

typedef struct{
    int data_int[6];//pid, priority, nice, virt, res, shr;
    double data_db[2];//cpu, mem
    char data_str[4][CMD_LEN];//user, state, time, cmd
    long long time_val;
}cusProc;

int col_hash[COLUMN_CNT] = {0, 2, 3, 4, 5, 6, 8, 9, 1, 7, 10, 11};

cusProc proc_list[PROC_CNT_MAX];
int proc_cnt, input, row, col, core_cnt;
long uptime, beftime = 0, mem_total, cs_cnt, cpu_time[PID_CNT_MAX];
time_t bef, now;
char buf[BUFFER_SIZE], my_tty[LEN];
long double bef_ticks[TICK_CNT] = {0, };
char col_name[COLUMN_CNT][10] = {"PID", "USER", "PR", "NI", "VIRT", "RES", "SHR", "S", "%CPU", "%MEM", "TIME+", "COMMAND"};
int col_gap[COLUMN_CNT] = {1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 1};
char col_data[PROC_CNT_MAX][COLUMN_CNT][LEN];
float period = 3.0;
int opt_mask = 0;
char *opt_value[3];
int opt_row = 5;
bool sort_def = true; //true -> des, false -> asc
int sort_mask = 0;
int sort_col;

long get_mem();
long get_uptime();
int get_core();
void proc_search();
void proc_add(char path[PATH_LEN]);
void proc_init(cusProc *proc);
void proc_erase();
void print_scr();
void error1(int ec, char *str);
void error2(int ec, char ch);
void error3(int ec);

int compare_cpu(const void *a, const void *b)
{
    cusProc *p1 = (cusProc*)a, *p2 = (cusProc*)b;

    if(p1->data_db[CPU_IDX] == p2->data_db[CPU_IDX])
        return p1->data_int[PID_IDX] - p2->data_int[PID_IDX];
    if(sort_def)
        return p2->data_db[CPU_IDX] - p1->data_db[CPU_IDX];
    else   
        return p1->data_db[CPU_IDX] - p2->data_db[CPU_IDX];
}

int compare_pid(const void *a, const void *b)
{
    cusProc *p1 = (cusProc*)a, *p2 = (cusProc*)b;

    if(sort_def)
        return p2->data_int[PID_IDX] - p1->data_int[PID_IDX];
    else   
        return p1->data_int[PID_IDX] - p2->data_int[PID_IDX];
}

int compare_mem(const void *a, const void *b)
{
    cusProc *p1 = (cusProc*)a, *p2 = (cusProc*)b;

    if(p1->data_db[MEM_IDX] == p2->data_db[MEM_IDX])
        return p1->data_int[PID_IDX] - p2->data_int[PID_IDX];
    if(sort_def)
        return p2->data_db[MEM_IDX] > p1->data_db[MEM_IDX];
    else   
        return p1->data_db[MEM_IDX] > p2->data_db[MEM_IDX];
}

int compare_time(const void *a, const void *b)
{
    cusProc *p1 = (cusProc*)a, *p2 = (cusProc*)b;

    if(p1->time_val == p2->time_val)
        return p1->data_int[PID_IDX] - p2->data_int[PID_IDX];
    if(sort_def)
        return p2->time_val > p1->time_val;
    else
        return p2->time_val < p1->time_val;
}

int main(int argc, char **argv)
{
    int len;
    for(int i = 1;i < argc;i++)
    {
        len = strlen(argv[i]);
        int j = 0;
        int opt =  -1;
        for(;j < len;j++)
        {
            switch(argv[i][j])
            {
                case 'd': opt_mask |= 1 << OPT_d; opt = OPT_d; break;
                case 'u':
                    opt_mask |= 1 << OPT_u;
                    if(opt_mask & 1 << OPT_p)
                        error3(2);
                    opt = OPT_u; break;
                case 'p':
                    opt_mask |= 1 << OPT_p;
                    if(opt_mask & 1 << OPT_u)
                        error3(2);
                    opt = OPT_p; break;
                case '-': break;
                default: error2(0, argv[i][j]); break;
            }
            if(opt != -1)
                break;
        }

        if(opt == -1)
        {
            if(len > 1)
                argv[i]++;
            error1(0, argv[i]);
        }
        else
        {
            if(j < len - 1)
                opt_value[opt] = argv[i] + j + 1;
            else
            {
                i++;
                if(i >= argc)
                    error2(1, argv[i - 1][j]);
                opt_value[opt] = argv[i];
            }
        }
    }

    if(opt_mask & 1 << OPT_d)
    {
        int d_len = strlen(opt_value[OPT_d]);
        for(int i = 0;i < d_len;i++)
        {
            if(!isdigit(opt_value[OPT_d][i]) || opt_value[OPT_d][i] != '.')
                error1(1, opt_value[OPT_d]);
        }
    }


    start_color();
    mem_total = get_mem();
    cs_cnt = sysconf(_SC_CLK_TCK);
    now = time(NULL);
    memset(cpu_time, 0, PID_CNT_MAX);
    core_cnt = get_core();

    initscr();
    keypad(stdscr, TRUE);
    noecho();
    halfdelay(10);
    curs_set(0);

    bool def = true;
    input = row = col = 0;

    char tc;
    int opt_cur;
    bool opt_def;

    bef = time(NULL);

    while((input = getch()) != 'q')
    {
        now = time(NULL);

        if(now - bef >= period)
        {
            def = true;
        }

        switch(input)
        {
            case KEY_UP:
                if(row > 0)
                    row--;
                def = true;
                break;
            case KEY_DOWN:
                if(row < proc_cnt)
                    row++;
                def = true;
                break;
            case KEY_LEFT:
                if(col > 0)
                    col--;
                def = true;
                break;
            case KEY_RIGHT:
                if(col < COLUMN_CNT - 1)
                    col++;
                def = true;
                break;
            case 'd':
            case 's':
                if(!(opt_mask & 1 << OPT_B))
                    attron(A_BOLD);
                mvprintw(opt_row, 0, "Change delay from %.1f to ", period);
                attroff(A_BOLD);
                cbreak();
                echo();
                curs_set(1);
                opt_value[OPT_d] = calloc(DATA_LEN, sizeof(char));
                opt_cur = 0;
                opt_def = false;
                while((tc = getch()) != '\n')
                {
                    if(('0' <= tc && tc <= '9') || tc == '.')
                    {
                        opt_value[OPT_d][opt_cur] = tc;
                        opt_cur++;
                    }
                    else
                    {
                        opt_def = true;
                    }
                }
                noecho();
                curs_set(0);
                halfdelay(10);
                if(opt_def)
                {
                    attron(A_REVERSE);
                    mvprintw(opt_row, 0, " Unacceptable floating point ");
                    attroff(A_REVERSE);
                    break;
                }
                def = true;
                now = time(NULL);
                opt_mask |= 1 << OPT_d;
                break;
            case 'u':
                if(!(opt_mask & 1 << OPT_B))
                    attron(A_BOLD);
                mvprintw(opt_row, 0, "Which user (blank for all) ");
                attroff(A_BOLD);
                cbreak();
                echo();
                curs_set(1);
                opt_value[OPT_u] = calloc(DATA_LEN, sizeof(char));
                opt_cur = 0;
                while((tc = getch()) != '\n')
                {
                    opt_value[OPT_u][opt_cur] = tc;
                    opt_cur++;
                }
                noecho();
                curs_set(0);
                halfdelay(10);
                def = true;
                opt_mask |= 1 << OPT_u;
                if(opt_cur == 0)
                {
                    opt_mask ^= 1 << OPT_u;
                }
                break;
            case 'B': opt_mask ^= 1 << OPT_B; def = true; break;
            case 'b': opt_mask ^= 1 << OPT_b; def = true; break;
            case 'x': opt_mask ^= 1 << OPT_x; def = true; break;
            case 'y': opt_mask ^= 1 << OPT_y; def = true; break;
            case 'l':
                opt_mask ^= 1 << OPT_l;
                if(opt_mask & 1 << OPT_l)
                    opt_row = 4;
                else
                    opt_row = 5;
                def = true;
                break;
            case 'R': opt_mask ^= 1 << OPT_R; def = true; sort_def ^= true; break;
            case 'P': sort_mask = 0; def = true; break;
            case 'M': sort_mask = 1 << SORT_M; def = true; break;
            case 'N': sort_mask = 1 << SORT_N; def = true; break;
            case 'T': sort_mask = 1 << SORT_T; def = true; break;
            case ' ': def = true; break;
            case -1: break;
            case 410: def = true; break;
        }

        if(def)
        {
            clear();
            proc_erase();
            proc_cnt = 0;
            proc_search();
            if(sort_mask == 0)
                qsort(proc_list, proc_cnt, sizeof(proc_list[0]), compare_cpu);
            else if(sort_mask & 1 << SORT_M)
                qsort(proc_list, proc_cnt, sizeof(proc_list[0]), compare_mem);
            else if(sort_mask & 1 << SORT_N)
                qsort(proc_list, proc_cnt, sizeof(proc_list[0]), compare_pid);
            else if(sort_mask & 1 << SORT_T)
                qsort(proc_list, proc_cnt, sizeof(proc_list[0]), compare_time);
            print_scr();
            refresh();
            bef = now;
            def = false;
        }

        if(opt_mask & 1 << OPT_d)
        {
            period = atof(opt_value[OPT_d]);
            opt_mask ^= 1 << OPT_d;
        }
    }

    endwin();

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

int get_core()
{
    FILE *fp;
    fp = fopen("/sys/devices/system/cpu/online", "r");

    memset(buf, 0, BUFFER_SIZE);
    fgets(buf, BUFFER_SIZE, fp);

    int res = atoi(buf + 2);

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
    strcpy(proc.data_str[STATE_IDX], stat_data[2]);

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

    if((opt_mask & 1 << OPT_u) && strcmp(proc.data_str[USER_IDX], opt_value[OPT_u]) != 0)
    {
        return;
    }

    double cpu;
    long long total = (long long)atoi(stat_data[13]) + (long long)atoi(stat_data[14]);
    proc.data_int[PR_IDX] = atoi(stat_data[17]);
    proc.data_int[NI_IDX] = atoi(stat_data[18]);
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
        proc.data_int[VIRT_IDX] = 0;
        proc.data_int[RES_IDX] = 0;
        proc.data_int[SHR_IDX] = 0;
        proc.data_db[MEM_IDX] = 0.0;
    }
    else
    {
        sscanf(cur, "%d", &proc.data_int[VIRT_IDX]);

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
        sscanf(cur, "%d", &proc.data_int[RES_IDX]);

        for(int i = 0;i < 2;i++)
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
        sscanf(cur, "%d", &proc.data_int[SHR_IDX]);

        double mem = (double)proc.data_int[RES_IDX] / mem_total * 100;

        if(isnan(mem) || isinf(mem) || mem > 100 || mem < 0)
            proc.data_db[MEM_IDX] = 0.0;
        else
            proc.data_db[MEM_IDX] = mem;
    }
    fclose(fp);

    long long minute = total / 3600;
    double rest = (double)(total % 3600) / 100.0;
    sprintf(proc.data_str[TIME_IDX], "%0lld:%05.2lf", minute, rest);
    proc.time_val = total;

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
                strcat(proc.data_str[CMD_IDX], " ");
        }
        strcat(proc.data_str[CMD_IDX], c);
    }

    if(strlen(proc.data_str[CMD_IDX]) == 0)
    {
        char tmp[CMD_LEN];
        sscanf(stat_data[1], "(%s", tmp);
        tmp[strlen(tmp) - 1] = 0;
        sprintf(proc.data_str[CMD_IDX], "%s", tmp);
    }

    fclose(fp);

    proc_list[proc_cnt] = proc;
    proc_cnt++;
}

void proc_init(cusProc *proc)
{
    memset(proc->data_int, 0, 6);
    memset(proc->data_db, 0.0, 2);
    for(int i = 0;i < 4;i++)
    {
        memset(proc->data_str[i], 0, CMD_LEN);
    }
    proc->time_val = 0;
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
    uptime = get_uptime();
    int output_row = 0;

    char now_str[128];
    memset(now_str, 0, 128);
    struct tm *now_tm = localtime(&now);
    sprintf(now_str, "top - %02d:%02d:%02d", now_tm->tm_hour, now_tm->tm_min, now_tm->tm_sec);

    char up_str[128];
    memset(up_str, 0, 128);
    struct tm* up_tm = localtime(&uptime);
    if(uptime < 60 * 60)
        sprintf(up_str, "%2d min", up_tm->tm_min);
    else if(uptime < 60 * 60 * 24)
        sprintf(up_str, "%2d:%02d", up_tm->tm_hour, up_tm->tm_min);
    else   
        sprintf(up_str, "%3d days, %02d:%02d", up_tm->tm_yday, up_tm->tm_hour, up_tm->tm_min);

    int user_cnt = 0;

    struct utmp *ucur;
    setutent();
    while((ucur = getutent()) != NULL)
    {
        if(ucur->ut_type == USER_PROCESS)
            user_cnt++;
    }

    FILE *fp;
    double loadavg[3];
    fp = fopen("/proc/loadavg", "r");
    memset(buf, 0, BUFFER_SIZE);
    fgets(buf, BUFFER_SIZE, fp);
    fclose(fp);
    sscanf(buf, "%lf%lf%lf", &loadavg[0], &loadavg[1], &loadavg[2]);

    if(!(opt_mask & 1 << OPT_l))
    {
        if(user_cnt == 1)
            mvprintw(output_row, 0, "%s up %s, %2d user,  load average: %2.2f, %2.2f, %2.2f", now_str, up_str, user_cnt, loadavg[0], loadavg[1], loadavg[2]);
        else
            mvprintw(output_row, 0, "%s up %s, %2d users, load average: %2.2f, %2.2f, %2.2f", now_str, up_str, user_cnt, loadavg[0], loadavg[1], loadavg[2]);
        output_row++;
    }

    int total = 0, running = 0, sleeping = 0, stopped = 0, zombie = 0;
    total = proc_cnt;
    for(int i = 0;i < proc_cnt;i++)
    {
        switch(proc_list[i].data_str[1][0])
        {
            case 'R': running++; break;
            case 'D':
            case 'S': sleeping++; break;
            case 'T':
            case 't': stopped++; break;
            case 'Z': zombie++; break;
        }
    }

    mvprintw(output_row, 0, "Tasks:     total,     running,     sleeping,     stopped,     zombie");

    if(!(opt_mask & 1 << OPT_B))
        attron(A_BOLD);

    mvprintw(output_row, 7, "%3d", total);
    mvprintw(output_row, 18, "%3d", running);
    mvprintw(output_row, 31, "%3d", sleeping);
    mvprintw(output_row, 45, "%3d", stopped);
    mvprintw(output_row, 58, "%3d", zombie);

    attroff(A_BOLD);

    output_row++;
    double us, sy, ni, id, wa, hi, si, st;

    fp = fopen("/proc/stat", "r");
    memset(buf, 0, BUFFER_SIZE);
    fgets(buf, BUFFER_SIZE, fp);
    fclose(fp);

    char *cur;
    for(int i = 0;;i++)
    {
        if(isdigit(buf[i]))
        {
            cur = buf + i;
            break;
        }
    }

    double tick_list[TICK_CNT] = {0.0, }, result[TICK_CNT];

    sscanf(cur, "%lf%lf%lf%lf%lf%lf%lf%lf", &tick_list[0], &tick_list[1], &tick_list[2], &tick_list[3], &tick_list[4], &tick_list[5], &tick_list[6], &tick_list[7]);

    long tick_total = (uptime - beftime) * cs_cnt;

    for(int i = 0;i < TICK_CNT;i++)
    {
        tick_list[i] /= core_cnt;
        result[i] = tick_list[i] - bef_ticks[i];
        result[i] = (result[i] / tick_total) * 100;
        if(isnan(result[i]) || isinf(result[i]))
            result[i] = 0;
        bef_ticks[i] = tick_list[i];
    }
    beftime = uptime;

    mvprintw(output_row, 0, "%%Cpu(s):      us,      sy,      ni,      id,      wa,      hi,      si,      st");
    
    if(!(opt_mask & 1 << OPT_B))
        attron(A_BOLD);

    for(int i = 0;i < 8;i++)
    {
        mvprintw(output_row, 9 + 9 * i, "%4.1f", result[i]);
    }
    attroff(A_BOLD);
    
    output_row++;

    double mem_info[8], mem_used, swap_used;

    fp = fopen("/proc/meminfo", "r");

    int idx = 0;

    for(int i = 0;i < 24;i++)
    {
        memset(buf, 0, BUFFER_SIZE);
        fgets(buf, BUFFER_SIZE, fp);
        if(i < 5 || i == 14 || i == 15 || i == 23)
        {
            for(int j = 0;;j++)
            {
                if(isdigit(buf[j]))
                {
                    cur = buf + j;
                    break;
                }
            }
            sscanf(cur, "%lf", &mem_info[idx]);
            mem_info[idx] /= 1000.0;
            idx++;
        }
    }

    fclose(fp);

    mem_used = mem_info[0] - mem_info[1] - mem_info[3] - mem_info[4] - mem_info[7];
    swap_used = mem_info[5] - mem_info[6];

    mvprintw(output_row, 0, "MiB Mem :          total,          free,          used,          buff/cache");

    if(!(opt_mask & 1 << OPT_B))
        attron(A_BOLD);

    mvprintw(output_row, 10, "%8.1f", mem_info[0]);
    mvprintw(output_row, 26, "%8.1f", mem_info[1]);
    mvprintw(output_row, 41, "%8.1f", mem_used);
    mvprintw(output_row, 56, "%8.1f", mem_info[3] + mem_info[4] + mem_info[7]);

    attroff(A_BOLD);

    output_row++;
    mvprintw(output_row, 0, "MiB Swap:          total,          free,          used,          avail Mem");

    if(!(opt_mask & 1 << OPT_B))
        attron(A_BOLD);

    mvprintw(output_row, 10, "%8.1f", mem_info[5]);
    mvprintw(output_row, 26, "%8.1f", mem_info[6]);
    mvprintw(output_row, 41, "%8.1f", swap_used);
    mvprintw(output_row, 56, "%8.1f", mem_info[2]);

    attroff(A_BOLD);

    output_row++;

    for(int i = 0;i < COLS;i++)
    {
        mvprintw(output_row, i, " ");
    }

    int col_width[COLUMN_CNT] = {3, 4, 2, 2, 4, 3, 3, 1, 4, 4, 5, 7};

    for(int i = 0;i < proc_cnt;i++)
    {
        int cur = 0;
        for(int j = 0;j < 6;j++)
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

        for(int j = 0;j < 3;j++)
        {
            strcpy(col_data[i][col_hash[cur]], proc_list[i].data_str[j]);
            if(col_width[col_hash[cur]] < strlen(proc_list[i].data_str[j]))
                col_width[col_hash[cur]] = strlen(proc_list[i].data_str[j]);
            cur++;
        }
        strcpy(col_data[i][col_hash[cur]], proc_list[i].data_str[3]);
    }

    int col_start[COLUMN_CNT] = {0, }, left = col, right = 0, cmd_len = -1;

    int i = col + 1;
    col_start[0] = 2;
    for(;i < COLUMN_CNT;i++)
    {
        col_start[i] = col_width[i - 1] + col_gap[i - 1] + col_start[i - 1];
        if(col_start[i] + col_width[i] >= COLS)
        {
            break;
        }
    }
    right = i;
    
    if(i == COLUMN_CNT)
    {
        cmd_len = COLS - col_start[11];
    }

    int gap = 0;

    output_row++;

    attron(A_REVERSE);

    for(int i = 0;i < COLS;i++)
    {
        mvprintw(output_row, i, " ");
    }

    for(int i = left;i < right;i++)
    {
        gap = col_width[i] - strlen(col_name[i]);
        if(i == 1 || i == 11)
        {
            mvprintw(output_row, col_start[i], "%s", col_name[i]);
        }
        else
        {
            mvprintw(output_row, col_start[i] + gap, "%s", col_name[i]);
        }
    }
    output_row++;

    attroff(A_REVERSE);

    if(opt_mask & 1 << OPT_x)
    {
        switch(sort_mask)
        {
            case 0: sort_col = 8; break;
            case 1 << SORT_M: sort_col = 9; break;
            case 1 << SORT_N: sort_col = 0; break;
            case 1 << SORT_T: sort_col = 10; break;
        }
    }

    for(int i = row;i < proc_cnt;i++)
    {
        if((opt_mask & 1 << OPT_u) && (strcmp(col_data[i][1], opt_value[OPT_u]) != 0))
        {
            output_row--;
            continue;
        }
        if((opt_mask & 1 << OPT_p) && (strcmp(col_data[i][0], opt_value[OPT_p]) != 0))
        {
            output_row--;
            continue;
        }

        if(col_data[i][7][0] == 'R' && !(opt_mask & 1 << OPT_y))
        {
            if(!(opt_mask & 1 << OPT_B))
                attron(A_BOLD);
            if(opt_mask & 1 << OPT_b)
            {
                attroff(A_BOLD);
                attron(A_REVERSE);
                for(int k = 0;k < COLS;k++)
                {
                    mvprintw(output_row + i - row, k, " ");
                }
            }
        }

        for(int j = left;j < right;j++)
        {
            if((opt_mask & 1 << OPT_x) && j == sort_col)
            {
                if(!(opt_mask & 1 << OPT_B))
                    attron(A_BOLD);
                if(opt_mask & 1 << OPT_b)
                {
                    attroff(A_BOLD);
                    attron(A_REVERSE);
                    int k = -1;
                    if(j == 0)
                        k--;
                    for(;k <= col_width[j];k++)
                    {
                        mvprintw(output_row + i - row, k + col_start[j], " ");
                    }
                }
            }

            gap = col_width[j] - strlen(col_data[i][j]);
            if(j == 1)
            {
                mvprintw(output_row + i - row, col_start[j], "%s", col_data[i][j]);
            }
            else if(j == 11)
            {
                if(strlen(col_data[i][j]) > cmd_len)
                {
                    col_data[i][j][cmd_len - 2] = '+';
                    col_data[i][j][cmd_len - 1] = 0;
                }
                mvprintw(output_row + i - row, col_start[j], "%s", col_data[i][j]);
            }
            else
            {
                mvprintw(output_row + i - row, col_start[j] + gap, "%s", col_data[i][j]);
            }

            if(!(col_data[i][7][0] == 'R' && !(opt_mask & 1 << OPT_y)))
            {
                if((opt_mask & 1 << OPT_x) && j == sort_col)
                {
                    if(!(opt_mask & 1 << OPT_B))
                        attroff(A_BOLD);
                    if(opt_mask & 1 << OPT_b)
                    {
                        attroff(A_REVERSE);
                    }
                }
            }
        }

        if(col_data[i][7][0] == 'R' && !(opt_mask & 1 << OPT_y))
        {
            if(!(opt_mask & 1 << OPT_B))
                attroff(A_BOLD);
            if(opt_mask & 1 << OPT_b)
            {
                attroff(A_REVERSE);
            }
        }
    }
}

void error1(int ec, char *str)
{
    switch(ec)
    {
        case 0: printf("mytop: inappropriate '%s'\n", str); break;
        case 1: printf("mytop: bad delay interval '%s'\n", str); break;
    }
    exit(1);
}

void error2(int ec, char ch)
{
    switch(ec)
    {
        case 0: printf("mytop: unknown option '%c'\n", ch); break;
        case 1: printf("mytop: -%c requires argument\n", ch); break;
    }
    exit(1);
}

void error3(int ec)
{
    switch(ec)
    {
        case 0: printf("mytop: Invalid user\n"); break;
        case 1: printf("mytop: Invalid pid\n"); break;
        case 2: printf("mytop: conflicting process selections (U/p/u)\n"); break;
    }
    exit(1);
}