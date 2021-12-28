#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <pthread.h>
#include <sys/types.h>

#define MAX_LEN 10000
#define NAME_LEN 30
#define READ 0
#define WRITE 1
#define KEY 20172621
#define KEYm 1128
#define BUFFER_SIZE 2

pthread_mutex_t lock1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock2 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond2 = PTHREAD_COND_INITIALIZER;

int m = 1, n = 1, def; //행, 열
char *ori;
struct timeval st_time, end_time;

struct msgbuf
{
    long msg_type;
    char msg_text[BUFFER_SIZE];
};

int d[2][8] = {{-1, -1, -1, 0, 0, 1, 1, 1}, {-1, 0, 1, -1, 1, -1, 0, 1}};

int *cpid, ppid;

pthread_t *ctid, ptid;
int def, def2, *tlen, *tst, *tend;
char *tbefmat, *tnextmat;

int cnt, pcnt, tcnt;

void getmat(FILE *);
void game(char *, char *, char *, int);
void writemat(FILE *, char *);
void sequential_way();
void process_way();
void *game_thread(void *arg);
void thread_way();
void print_info(int);

int main(int argc, char **argv)
{
    FILE *fp = fopen(argv[1], "r");
    if(fp == NULL)
    {
        printf("No file!\n");
        return 0;
    }

    getmat(fp);
    fclose(fp);

    while(1)
    {
        int cmd = 0;
        printf("(1) 프로그램 종료 (2) 순차처리 (3) Process 병렬처리 (4) Thread 병렬처리\n");
        printf("명령 번호 : ");
        scanf("%d", &cmd);
        
        if(cmd == 1)
        {
            pthread_mutex_destroy(&lock1);
            pthread_cond_destroy(&cond1);
            pthread_mutex_destroy(&lock2);
            pthread_cond_destroy(&cond2);
            printf("프로그램을 종료합니다.\n\n");
            break;
        }
        else if(cmd == 2)
        {
            printf("순차처리를 수행합니다.\n");
            cnt = 0;
            printf("수행 횟수 : ");
            scanf("%d", &cnt);
            sequential_way(cnt);
            print_info(1);
        }
        else if(cmd == 3)
        {
            printf("Process 병렬처리를 수행합니다.\n");
            cnt = 0;
            pcnt = 0;
            printf("수행 횟수 : ");
            scanf("%d", &cnt);
            printf("프로세스 개수 : ");
            scanf("%d", &pcnt);
            process_way(cnt);
            print_info(2);
        }
        else if(cmd == 4)
        {
            printf("Thread 병렬처리를 수행합니다.\n");
            cnt = 0;
            tcnt = 0;
            printf("수행 횟수 : ");
            scanf("%d", &cnt);
            printf("스레드 개수 : ");
            scanf("%d", &tcnt);
            thread_way(cnt);
            print_info(3);
        }
        else
        {
            printf("Wrong Command!\n");
        }
    }
}

void getmat(FILE *fp)
{
    char *buf = malloc(sizeof(char) * (MAX_LEN * 2 + 1));
    
    fseek(fp, 0, SEEK_SET);
    fgets(buf, (MAX_LEN * 2 + 1), fp);
    int len = (int)strlen(buf);
    n = len / 2;

    fseek(fp, 0, SEEK_SET);
    while(1)
    {
        if(fgets(buf, len + 1, fp) == NULL)
            break;
        m++;
    }
    m--;

    ori = malloc(sizeof(char) * (n + 2) * (m + 2));
    char *cur = ori;
    fseek(fp, 0, SEEK_SET);
    for(int i = 0; i < m + 2;i++)
    {
        cur = ori + i * (n + 2);
        memset(cur, '0', sizeof(char) * (n + 2));

        if(i == 0 || i == m + 1)
            continue;

        fgets(buf, len + 1, fp);
        for(int j = 1;j <= n;j++)
        {
            cur[j] = buf[(j - 1) * 2];
        }
    }
}

void game(char *befmat, char *cntmat, char *nextmat, int len)
{
    int curi, curj, curk;
    memset(cntmat, '0', sizeof(char) * (n + 2) * len);
    memset(nextmat, '0', sizeof(char) * (n + 2) * len);
    for(int i = 0;i < len;i++)
    {
        curi = i * (n + 2);
        for(int j = 1;j <= n;j++)
        {
            curj = curi + j;
            for(int k = 0;k < 8;k++)
            {
                curk = (i + d[0][k]) * (n + 2) + j + d[1][k];
                cntmat[curj] += befmat[curk] -'0';
            }
        }

        for(int j = 1;j <= n;j++)
        {
            curj = curi + j;
            if(befmat[curj] == '1')
            {
                if('3' <= cntmat[curj] && cntmat[curj] <= '6')
                {
                    nextmat[curj] = '1';
                }
            }
            else
            {
                if(cntmat[curj] == '4')
                {
                    nextmat[curj] = '1';
                }
            }
        }
    }
}

void writemat(FILE *fp, char *mat)
{
    fseek(fp, 0, SEEK_SET);
    char buf[3];
    int curi;
    for(int i = 1;i <= m;i++)
    {
        curi = i * (n + 2);
        for(int j = 1;j <= n;j++)
        {
            sprintf(buf, "%c ", mat[curi + j]);
            fwrite(buf, sizeof(char) * 2, 1, fp);
        }
        if(i != m)
        {
            sprintf(buf, "\n");
            fwrite(buf, sizeof(char) * 1, 1, fp);
        }
    }
}

void sequential_way()
{
    char fname[NAME_LEN];
    FILE *newfp;

    char *befmat = malloc(sizeof(char) * (n + 2) * (m + 2));
    char *nextmat = malloc(sizeof(char) * (n + 2) * (m + 2));
    char *cntmat = malloc(sizeof(char) * (n + 2) * (m + 2));

    memset(befmat, '0', sizeof(char) * (n + 2) * (m + 2));
    memset(nextmat, '0', sizeof(char) * (n + 2) * (m + 2));
    memset(cntmat , '0', sizeof(char) * (n + 2) * (m + 2));

    gettimeofday(&st_time, NULL);
    memcpy(befmat, ori, sizeof(char) * (n + 2) * (m + 2));

    for(int num = 1;num <= cnt;num++)
    {
        game(befmat + (n + 2), cntmat + (n + 2), nextmat + (n + 2), m);

        memset(fname, 0x00, NAME_LEN);
        sprintf(fname, "gen_%d.matrix", num);
        if(num == cnt)
            sprintf(fname, "output.matrix");
        newfp = fopen(fname, "w+");

        writemat(newfp, nextmat);
        
        fclose(newfp);
        if(num == cnt)
            gettimeofday(&end_time, NULL);

        memcpy(befmat, nextmat, sizeof(char) * (n + 2) * (m + 2));
    }
}

void process_way()
{
    char fname[NAME_LEN];
    FILE *newfp;

    ppid = getpid();
    cpid = malloc(sizeof(int) * pcnt);

    int *plen = malloc(sizeof(int) * pcnt), tmp = m / pcnt;
    for(int i = 0;i < pcnt;i++)
    {
        plen[i] = tmp;
    }
    tmp = m % pcnt;
    for(int i = 0;i < tmp;i++)
    {
        plen[i]++;
    }

    int *pst = malloc(sizeof(int) * (pcnt + 1));
    int *pend = malloc(sizeof(int) * (pcnt + 1));
    pst[0] = 1;
    for(int i = 1;i <= pcnt;i++)
    {
        pst[i] = pst[i - 1] + plen[i - 1];
        pend[i - 1] = pst[i];
    }
    pend[pcnt] = 0;

    struct msgbuf mb;
    memset(mb.msg_text, 0x00, BUFFER_SIZE);
    mb.msg_type = 1;
    mb.msg_text[0] = '1';
    int mid = msgget(KEYm, IPC_CREAT|0644);
    int mid2 = msgget(KEYm + 1, IPC_CREAT|0644);

    char *befmat, *nextmat, *cntmat, *resmat;
    int shid;
    shid = shmget((key_t)KEY, sizeof(char) * (n + 2) * (m + 2), IPC_CREAT|0644);
    befmat = shmat(shid, (void *)0, 0);
    shmctl(shid, IPC_RMID, (void *)0);
    memset(befmat, '0', sizeof(char) * (n + 2) * (m + 2));
    
    shid = shmget((key_t)KEY + 1, sizeof(char) * (n + 2) * (m + 2), IPC_CREAT|0644);
    nextmat = shmat(shid, (void *)0, 0);
    shmctl(shid, IPC_RMID, (void *)0);
    memset(nextmat, '0', sizeof(char) * (n + 2) * (m + 2));
    
    cntmat = malloc(sizeof(char) * (n + 2) * (m + 2));
    memset(cntmat, '0', sizeof(char) * (n + 2) * (m + 2));

    resmat = malloc(sizeof(char) * (n + 2) * (m + 2));
    memset(resmat, '0', sizeof(char) * (n + 2) * (m + 2));

    gettimeofday(&st_time, NULL);
    memcpy(befmat, ori, sizeof(char) * (n + 2) * (m + 2));

    int curi;
    for(int pnum = 0;pnum < pcnt;pnum++)
    {
        if((cpid[pnum] = fork()) == 0)
        {
            for(int num = 1;num <= cnt;num++)
            {
                game(befmat + pst[pnum] * (n + 2), cntmat + pst[pnum] * (n + 2), nextmat + pst[pnum] * (n + 2), plen[pnum]);
                
                mb.msg_type = 1;
                msgsnd(mid, &mb, sizeof(mb.msg_text), 0);
                msgrcv(mid, &mb, sizeof(mb.msg_text), 2, 0);

                curi = pst[pnum] * (n + 2);
                memcpy(befmat + curi, nextmat + curi, sizeof(char) * (n + 2) * plen[pnum]);

                mb.msg_type = 1;
                msgsnd(mid2, &mb, sizeof(mb.msg_text), 0);
                msgrcv(mid2, &mb, sizeof(mb.msg_text), 2, 0);
            }

            exit(0);
        }
    }

    for(int num = 1;num <= cnt;num++)
    {
        for(int i = 0;i < pcnt;i++)
        {
            msgrcv(mid, &mb, sizeof(mb.msg_text), 1, 0);
        }

        memcpy(resmat, nextmat, sizeof(char) * (n + 2) * (m + 2));

        for(int i = 0;i < pcnt;i++)
        {
            mb.msg_type = 2;
            msgsnd(mid, &mb, sizeof(mb.msg_text), 0);
        }
        
        memset(fname, 0x00, NAME_LEN);
        sprintf(fname, "gen_%d.matrix", num);
        if(num == cnt)
            sprintf(fname, "output.matrix");
        newfp = fopen(fname, "w+");

        writemat(newfp, resmat);
        fclose(newfp);
        
        if(num == cnt)
            gettimeofday(&end_time, NULL);

        for(int i = 0;i < pcnt;i++)
        {
            msgrcv(mid2, &mb, sizeof(mb.msg_text), 1, 0);
        }

        for(int i = 0;i < pcnt;i++)
        {
            mb.msg_type = 2;
            msgsnd(mid2, &mb, sizeof(mb.msg_text), 0);
        }
    }

    int status;
    for(int i = 0;i < pcnt;i++)
    {
        int t = wait(&status);
    }

    shmdt(befmat);
    shmdt(nextmat);

    msgctl(mid, IPC_RMID, NULL);
    msgctl(mid2, IPC_RMID, NULL);
}

void *game_thread(void *arg)
{
    int tnum = *((int *)arg);
    int curmat, curi, curj, curk;
    char *curc = malloc(sizeof(char) * (n + 2));

    for(int num = 1;num <= cnt;num++)
    {
        for(int i = tst[tnum];i < tend[tnum];i++)
        {
            curi = i * (n + 2);
            memset(curc, '0', sizeof(char) * (n + 2));
            for(int j = 1;j <= n;j++)
            {
                for(int k = 0;k < 8;k++)
                {
                    curk = (i + d[0][k]) * (n + 2) + j + d[1][k];
                    curc[j] += tbefmat[curk] - '0';
                }
            }

            memset(tnextmat + curi, '0', sizeof(char) * (n + 2));

            for(int j = 1;j <= n;j++)
            {
                curj = curi + j;
                if(tbefmat[curj] == '1')
                {
                    if('3' <= curc[j] && curc[j] <= '6')
                    {
                        tnextmat[curj] = '1';
                    }
                }
                else
                {
                    if(curc[j] == '4')
                    {
                        tnextmat[curj] = '1';
                    }
                }
            }
        }

        curmat = tst[tnum] * (n + 2);

        pthread_mutex_lock(&lock1);
        def++;
        pthread_cond_wait(&cond1, &lock1);
        def--;
        
        memcpy(tbefmat + curmat, tnextmat + curmat, sizeof(char) * (n + 2) * tlen[tnum]);

        def2++;
        pthread_cond_wait(&cond2, &lock1);
        def2--;

        pthread_mutex_unlock(&lock1);
    }
}

void thread_way()
{
    char fname[NAME_LEN];
    FILE *newfp;

    ptid = pthread_self();
    ctid = malloc(sizeof(pthread_t) * tcnt);
    def = 0;
    def2 = 0;
    
    tlen = malloc(sizeof(int) * tcnt);
    int tmp = m / tcnt;
    for(int i = 0;i < tcnt;i++)
    {
        tlen[i] = tmp;
    }
    tmp = m % tcnt;
    for(int i = 0;i < tmp;i++)
    {
        tlen[i]++;
    }
    
    tst = malloc(sizeof(int) * (tcnt + 1));
    tend = malloc(sizeof(int) * (tcnt + 1));
    tst[0] = 1;
    for(int i = 1;i <= tcnt;i++)
    {
        tst[i] = tst[i - 1] + tlen[i - 1];
        tend[i - 1] = tst[i];
    }
    tend[tcnt] = 0;
    
    
    tbefmat = malloc(sizeof(char) * (n + 2) * (m + 2));
    memset(tbefmat, '0', sizeof(char) * (n + 2) * (m + 2));
    tnextmat = malloc(sizeof(char) * (n + 2) * (m + 2));
    memset(tnextmat, '0', sizeof(char) * (n + 2) * (m + 2));

    char *tresmat = malloc(sizeof(char) * (n + 2) * (m + 2));
    memset(tresmat, '0', sizeof(char) * (n + 2) * (m + 2));

    gettimeofday(&st_time, NULL);
    
    memcpy(tbefmat, ori, sizeof(char) * (n + 2) * (m + 2));

    int *tid = malloc(sizeof(int) * tcnt);
    for(int i = 0;i < tcnt;i++)
    {
        tid[i] = i;
        pthread_create(&ctid[i], NULL, game_thread, (void *)&tid[i]);
    }

    for(int num = 1;num <= cnt;num++)
    {
        pthread_mutex_lock(&lock2);

        while(def != tcnt) {}

        memset(fname, 0x00, NAME_LEN);
        sprintf(fname, "gen_%d.matrix", num);
        if(num == cnt)
            sprintf(fname, "output.matrix");
        newfp = fopen(fname, "w+");

        writemat(newfp, tnextmat);
        
        if(num == cnt)
            gettimeofday(&end_time, NULL);

        fclose(newfp);

        while(def > 0)
        {
            pthread_cond_broadcast(&cond1);
        }

        while(def2 != tcnt) {}

        while(def2 > 0)
        {
            pthread_cond_broadcast(&cond2);
        }
        
        pthread_mutex_unlock(&lock2);
    }

    int status;
    for(int i = 0;i < tcnt;i++)
    {
        pthread_join(ctid[i], (void *)&status);
    }
}

void print_info(int num)
{
    printf("\n");
    if(num == 2)
    {
        printf("부모 프로세스 ID : %d\n", ppid);
        for(int i = 0;i < pcnt;i++)
        {
            printf("%d번째 자식 프로세스 ID : %d\n", i + 1, cpid[i]);
        }
    }
    else if(num == 3)
    {
        printf("메인 스레드 ID : %u\n", (unsigned int)ptid);
        for(int i = 0;i < tcnt;i++)
        {
            printf("%d번째 생성된 스레드 ID : %u\n", i + 1, (unsigned int)ctid[i]);
        }
    }

    printf("소요 시간 : %ld ms\n\n", (end_time.tv_sec - st_time.tv_sec) * 1000 + (end_time.tv_usec - st_time.tv_usec) / 1000);
}