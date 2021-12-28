#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

sem_t rsem[5], psem, wsem;

int def = 1;
int ready[5] = {0,};
int passed[20], pcur = 0;
int waited[20], wcur = 0;
int garo_cnt = 0, sero_cnt = 0;
int garo_wait = 0, sero_wait = 0;

void *func1();
void *func2();
void *func3();
void *func4();

int main()
{
    int number;
    int road[15], rcur = -1;
    int tick = 0;
    int sum[5] = {0,};

    printf("Total number of vehicles : ");
    scanf("%d", &number);

    printf("Start point : ");

    srand(time(NULL));
    for(int i = 0;i < number;i++)
    {
        road[i] = rand() % 4 + 1;
        printf("%d ", road[i]);
    }
    printf("\n");
    
    for(int i = 1;i < 5;i++)
    {
        sem_init(&rsem[i], 0, 0);
    }
    sem_init(&psem, 0, 1);
    sem_init(&wsem, 0, 1);

    pthread_t tid[4];
    pthread_create(&tid[0], NULL, func1, NULL);
    pthread_create(&tid[1], NULL, func2, NULL);
    pthread_create(&tid[2], NULL, func3, NULL);
    pthread_create(&tid[3], NULL, func4, NULL);

    sleep(1);

    int tmp;

    while(def)
    {
        tick++;

        if(rcur < number)
            rcur++;

        pcur = 0;

        if(rcur < number)
        {
            waited[wcur] = road[rcur];
            wcur++;
            ready[road[rcur]]++;
            if(road[rcur] % 2 == 0)
                garo_wait++;
            else
                sero_wait++;
        }

        if(garo_cnt == 0 && sero_cnt == 0)
        {
            if(garo_wait == 0 && sero_wait == 0)
            {
                def = 0;
            }
            if(garo_wait > 0 && sero_wait > 0)
            {
                tmp = rand() % 2;
                if(tmp == 1)
                {
                    if(ready[2] > 0 && ready[4] > 0)
                    {
                        tmp = rand() % 2;
                        if(tmp == 1)
                        {
                            sem_post(&rsem[2]);
                        }
                        else
                        {
                            sem_post(&rsem[4]);
                        }
                    }
                    else if(ready[2] > 0)
                    {
                        sem_post(&rsem[2]);
                    }
                    else if(ready[4] > 0)
                    {
                        sem_post(&rsem[4]);
                    }
                }
                else
                {
                    if(ready[1] > 0 && ready[3] > 0)
                    {
                        tmp = rand() % 2;
                        if(tmp == 1)
                        {
                            sem_post(&rsem[1]);
                        }
                        else
                        {
                            sem_post(&rsem[3]);
                        }
                    }
                    else if(ready[1] > 0)
                    {
                        sem_post(&rsem[1]);
                    }
                    else if(ready[3] > 0)
                    {
                        sem_post(&rsem[3]);
                    }
                }
            }
            else if(garo_wait > 0)
            {
                if(ready[2] > 0 && ready[4] > 0)
                {
                    tmp = rand() % 2;
                    if(tmp == 1)
                    {
                        sem_post(&rsem[2]);
                    }
                    else
                    {
                        sem_post(&rsem[4]);
                    }
                }
                else if(ready[2] > 0)
                {
                    sem_post(&rsem[2]);
                }
                else if(ready[4] > 0)
                {
                    sem_post(&rsem[4]);
                }
            }
            else if(sero_wait > 0)
            {
                if(ready[1] > 0 && ready[3] > 0)
                {
                    tmp = rand() % 2;
                    if(tmp == 1)
                    {
                        sem_post(&rsem[1]);
                    }
                    else
                    {
                        sem_post(&rsem[3]);
                    }
                }
                else if(ready[1] > 0)
                {
                    sem_post(&rsem[1]);
                }
                else if(ready[3] > 0)
                {
                    sem_post(&rsem[3]);
                }
            }
        }
        else if(garo_cnt > 0)
        {
            sem_post(&rsem[2]);
            sem_post(&rsem[4]);
        }
        else if(sero_cnt > 0)
        {
            sem_post(&rsem[1]);
            sem_post(&rsem[3]);
        }

        sleep(1);

        printf("tick : %d\n", tick);
        printf("===============================\n");
        printf("Passed Vehicle\nCar ");

        for(int i = 0;i < pcur;i++)
        {
            printf("%d ", passed[i]);
            sum[passed[i]]++;
        }

        printf("\nWaiting Vehicle\nCar ");

        for(int i = 0;i < wcur;i++)
        {
            if(waited[i] > 0)
            {
                printf("%d ", waited[i]);
            }
        }

        printf("\n===============================\n");

    }

    for(int i = 0;i < 4;i++)
    {
        sem_post(&rsem[i + 1]);
        pthread_join(tid[i], NULL);
    }
    
    for(int i = 0;i < 5;i++)
    {
        sem_destroy(&rsem[i]);
    }
    sem_destroy(&psem);
    sem_destroy(&wsem);

    printf("Number of vehicles passed from each start point\n");
    for(int i = 1;i < 5;i++)
    {
        printf("P%d : %d times\n", i, sum[i]);
    }
    printf("Total time : %d ticks\n", tick);
}

void *func1()
{
    while(def)
    {
        sem_wait(&rsem[1]);
        
        if(ready[1] == 0)
            continue;
        ready[1]--;

        sem_wait(&wsem);

        for(int i = 0;i < wcur;i++)
        {
            if(waited[i] == 1)
            {
                waited[i] = 0;
                sero_wait--;
                sero_cnt++;
                break;
            }
        }

        sem_post(&wsem);

        sem_wait(&rsem[1]);
        
        sem_wait(&psem);

        passed[pcur] = 1;
        pcur++;
        sero_cnt--;

        sem_post(&psem);
    }
}

void *func2()
{
    while(def)
    {
        sem_wait(&rsem[2]);
        
        if(ready[2] == 0)
            continue;
        ready[2]--;

        sem_wait(&wsem);

        for(int i = 0;i < wcur;i++)
        {
            if(waited[i] == 2)
            {
                waited[i] = 0;
                garo_wait--;
                garo_cnt++;
                break;
            }
        }

        sem_post(&wsem);

        sem_wait(&rsem[2]);
        
        sem_wait(&psem);

        passed[pcur] = 2;
        pcur++;
        garo_cnt--;

        sem_post(&psem);
    }
}

void *func3()
{
    while(def)
    {
        sem_wait(&rsem[3]);
        
        if(ready[3] == 0)
            continue;
        ready[3]--;

        sem_wait(&wsem);

        for(int i = 0;i < wcur;i++)
        {
            if(waited[i] == 3)
            {
                waited[i] = 0;
                sero_wait--;
                sero_cnt++;
                break;
            }
        }

        sem_post(&wsem);

        sem_wait(&rsem[3]);
        
        sem_wait(&psem);

        passed[pcur] = 3;
        pcur++;
        sero_cnt--;

        sem_post(&psem);
    }
}

void *func4()
{
    while(def)
    {
        sem_wait(&rsem[4]);
        
        if(ready[4] == 0)
            continue;
        ready[4]--;

        sem_wait(&wsem);

        for(int i = 0;i < wcur;i++)
        {
            if(waited[i] == 4)
            {
                waited[i] = 0;
                garo_wait--;
                garo_cnt++;
                break;
            }
        }

        sem_post(&wsem);
        
        sem_wait(&rsem[4]);
        
        sem_wait(&psem);

        passed[pcur] = 4;
        pcur++;
        garo_cnt--;

        sem_post(&psem);
    }
}