#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LEN 20

int frame_cnt, page_ref[30] = {}, ref_cnt = 0;

void method_opt();
void method_fifo();
void method_lru();
void method_sc();

int main()
{
    char filename[LEN] = {};
    FILE *fp;    

    while(1)
    {
        printf("Type file name : ");
        scanf("%s", filename);

        if((fp = fopen(filename, "r")) == NULL)
            printf("\nNo such file. Retype file name.\n\n");
        else
            break;
    }

    fscanf(fp, "%d", &frame_cnt);
    while((fscanf(fp, "%d", page_ref+ref_cnt)) != EOF)
        ref_cnt++;

    printf("\n");
    while(1)
    {
        printf("Used method : ");
        char method_name[LEN] = {};
        scanf("%s", method_name);
        getchar();
        if(strcmp(method_name, "quit") == 0)
        {
            printf("\nprogram terminated.\n\n");
            break;
        }
        else if(strcmp(method_name, "OPT") == 0)
        {
            method_opt();
        }
        else if(strcmp(method_name, "FIFO") == 0)
        {
            method_fifo();
        }
        else if(strcmp(method_name, "LRU") == 0)
        {
            method_lru();
        }
        else if(strcmp(method_name, "Second-Chance") == 0)
        {
            method_sc();
        }
        else
        {
            printf("\nWrong command. Type again.\n\n");
        }
    }

    fclose(fp);

    return 0;
}

void method_opt()
{
    printf("page reference string : ");
    for(int i = 0;i < ref_cnt;i++)
    {
        printf("%d ", page_ref[i]);
    }
    printf("\n\n");

    int def_fault, fault_cnt = 0, *frame_st = calloc(frame_cnt, sizeof(int));

    printf("\tframe");
    for(int i = 1;i <= frame_cnt;i++)
    {
        printf("\t%d", i);
    }
    printf("\tpage fault\ntime\n");

    for(int i = 0;i < ref_cnt;i++)
    {
        def_fault = 1;
        for(int j = 0;j < frame_cnt;j++)
        {
            if(frame_st[j] == page_ref[i])
            {
                def_fault = 0;
                break;
            }
        }

        if(def_fault)
        {
            int def_skip = 0;
            for(int j = 0;j < frame_cnt;j++)
            {
                if(frame_st[j] == 0)
                {
                    frame_st[j] = page_ref[i];
                    def_skip = 1;
                    break;
                }
            }
            if(def_skip == 0)
            {
                int target, len = 0;

                for(int j = 0;j < frame_cnt;j++)
                {
                    int cur = i + 1;
                    for(;cur < ref_cnt;cur++)
                    {
                        if(frame_st[j] == page_ref[cur])
                        {
                            if(len < cur - i)
                            {
                                len = cur - i;
                                target = j;
                            }
                            break;
                        }
                    }
                    if(cur == ref_cnt)
                    {
                        target = j;
                        break;
                    }
                }

                frame_st[target] = page_ref[i];
            }
        }

        printf("%d\t\t", i + 1);
        for(int j = 0;j < frame_cnt;j++)
        {
            if(frame_st[j] != 0)
            {
                printf("%d", frame_st[j]);
            }
            printf("\t");
        }
        if(def_fault != 0)
        {
            printf("F");
            fault_cnt++;
        }
        printf("\n");
    }
    printf("Number of page faults : %d times\n\n", fault_cnt);
}

void method_fifo()
{
    printf("page reference string : ");
    for(int i = 0;i < ref_cnt;i++)
    {
        printf("%d ", page_ref[i]);
    }
    printf("\n\n");

    int def_fault, fault_cnt = 0, *frame_st = calloc(frame_cnt, sizeof(int));

    printf("\tframe");
    for(int i = 1;i <= frame_cnt;i++)
    {
        printf("\t%d", i);
    }
    printf("\tpage fault\ntime\n");

    int end_q = 0, target = 0, *queue = malloc(sizeof(int) * ref_cnt);
    memset(queue, -1, ref_cnt);

    for(int i = 0;i < ref_cnt;i++)
    {
        def_fault = 1;
        for(int j = 0;j < frame_cnt;j++)
        {
            if(frame_st[j] == page_ref[i])
            {
                def_fault = 0;
                break;
            }
        }

        if(def_fault)
        {
            int def_skip = 0;
            for(int j = 0;j < frame_cnt;j++)
            {
                if(frame_st[j] == 0)
                {
                    frame_st[j] = page_ref[i];
                    def_skip = 1;
                    queue[end_q] = j;
                    end_q++;
                    break;
                }
            }
            if(def_skip == 0)
            {
                frame_st[queue[target]] = page_ref[i];
                queue[end_q] = queue[target];
                end_q++;
                target++;
            }
        }

        printf("%d\t\t", i + 1);
        for(int j = 0;j < frame_cnt;j++)
        {
            if(frame_st[j] != 0)
            {
                printf("%d", frame_st[j]);
            }
            printf("\t");
        }
        if(def_fault != 0)
        {
            printf("F");
            fault_cnt++;
        }
        printf("\n");
    }
    printf("Number of page faults : %d times\n\n", fault_cnt);
}

void method_lru()
{
    printf("page reference string : ");
    for(int i = 0;i < ref_cnt;i++)
    {
        printf("%d ", page_ref[i]);
    }
    printf("\n\n");

    int def_fault, fault_cnt = 0, *frame_st = calloc(frame_cnt, sizeof(int));

    printf("\tframe");
    for(int i = 1;i <= frame_cnt;i++)
    {
        printf("\t%d", i);
    }
    printf("\tpage fault\ntime\n");

    for(int i = 0;i < ref_cnt;i++)
    {
        def_fault = 1;
        for(int j = 0;j < frame_cnt;j++)
        {
            if(frame_st[j] == page_ref[i])
            {
                def_fault = 0;
                break;
            }
        }

        if(def_fault)
        {
            int def_skip = 0;
            for(int j = 0;j < frame_cnt;j++)
            {
                if(frame_st[j] == 0)
                {
                    frame_st[j] = page_ref[i];
                    def_skip = 1;
                    break;
                }
            }
            if(def_skip == 0)
            {
                int target, len = 0;

                for(int j = 0;j < frame_cnt;j++)
                {
                    int cur = i - 1;
                    for(;cur >= 0;cur--)
                    {
                        if(frame_st[j] == page_ref[cur])
                        {
                            if(len < i - cur)
                            {
                                len = i - cur;
                                target = j;
                            }
                            break;
                        }
                    }
                }

                frame_st[target] = page_ref[i];
            }
        }

        printf("%d\t\t", i + 1);
        for(int j = 0;j < frame_cnt;j++)
        {
            if(frame_st[j] != 0)
            {
                printf("%d", frame_st[j]);
            }
            printf("\t");
        }
        if(def_fault != 0)
        {
            printf("F");
            fault_cnt++;
        }
        printf("\n");
    }
    printf("Number of page faults : %d times\n\n", fault_cnt);
}

void method_sc()
{
    printf("page reference string : ");
    for(int i = 0;i < ref_cnt;i++)
    {
        printf("%d ", page_ref[i]);
    }
    printf("\n\n");

    int def_fault, fault_cnt = 0, *frame_st = calloc(frame_cnt, sizeof(int));

    printf("\tframe");
    for(int i = 1;i <= frame_cnt;i++)
    {
        printf("\t%d", i);
    }
    printf("\tpage fault\ntime\n");

    int end_q = 0, *queue = malloc(sizeof(int) * ref_cnt), *hit = calloc(frame_cnt, sizeof(int));
    memset(queue, -1, ref_cnt);

    for(int i = 0;i < ref_cnt;i++)
    {
        def_fault = 1;
        for(int j = 0;j < frame_cnt;j++)
        {
            if(frame_st[j] == page_ref[i])
            {
                def_fault = 0;
                hit[j]++;
                break;
            }
        }

        if(def_fault)
        {
            int def_skip = 0;
            for(int j = 0;j < frame_cnt;j++)
            {
                if(frame_st[j] == 0)
                {
                    frame_st[j] = page_ref[i];
                    def_skip = 1;
                    queue[end_q] = j;
                    end_q++;
                    break;
                }
            }
            if(def_skip == 0)
            {
                for(int cur_q = 0;cur_q < end_q;cur_q++)
                {
                    if(queue[cur_q] == -1)
                        continue;
                    if(hit[queue[cur_q]] > 0)
                    {
                        hit[queue[cur_q]]--;
                        queue[end_q] = queue[cur_q];
                        queue[cur_q] = -1;
                        end_q++;
                    }
                    else
                    {
                        frame_st[queue[cur_q]] = page_ref[i];
                        queue[end_q] = queue[cur_q];
                        queue[cur_q] = -1;
                        end_q++;
                        break;
                    }
                }
            }
        }

        printf("%d\t\t", i + 1);
        for(int j = 0;j < frame_cnt;j++)
        {
            if(frame_st[j] != 0)
            {
                printf("%d", frame_st[j]);
            }
            printf("\t");
        }
        if(def_fault != 0)
        {
            printf("F");
            fault_cnt++;
        }
        printf("\n");
    }
    printf("Number of page faults : %d times\n\n", fault_cnt);
}