#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main()
{
    pid_t cpid[21], ppid = getpid(), tpid;
    int status;

    for(int i = 0;i < 21;i++)
    {
        if((cpid[i] = fork()) == 0)
        {
            printf("%d process begins\n", getpid());

            int **p = malloc(sizeof(int *) * 10000);
            for(int j = 0;j < 10000;j++)
            {
                p[j] = malloc(sizeof(int) * 2000);
                for(int k = 0;k < 2000;k++)
                {
                    p[j][k] = i * j * k;
                }
            }

            printf("%d process ends\n", getpid());

            exit(0);
        }
    }

    for(int i = 0;i < 21;i++)
    {
        tpid = wait(&status);
    }
    printf("----- All processes end -----\n");

    return 0;
}