#include <stdio.h>
#include <linux/kernel.h>
#include <sys/syscall.h>
#include <unistd.h>

long int value;
char buf = ' ', operator;
void get_token()
{
	while(buf == ' ')
	{
		buf = getchar();
	}

	if('0' <= buf && buf <= '9')
	{
		value = 0;
		while('0' <= buf && buf <= '9')
		{
			value = value * 10 + buf - '0';
			buf = getchar();
		}
	}
	else
	{
		operator = buf;
		buf = getchar();
	}
}

int main()
{
	get_token();
	long int tmp = value, res;
	get_token();
	get_token();

	switch(operator)
	{
		case '+': res = syscall(442, tmp, value); printf("%ld + %ld = %ld\n", tmp, value, res);break;
		case '-':
			if(tmp >= value)
				res = syscall(443, tmp, value);
			else
				res = -1 * syscall(443, value, tmp);	
			printf("%ld - %ld = %ld\n", tmp, value, res);break;
		case '*': res = syscall(444, tmp, value); printf("%ld * %ld = %ld\n", tmp, value, res);break;
		case '%': res = syscall(445, tmp, value); printf("%ld %% %ld = %ld\n", tmp, value, res);break;
		default: printf("error!\n"); break;
	}

	return 0;
}