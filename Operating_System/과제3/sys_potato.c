#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/syscalls.h>

asmlinkage long int sys_potato_plus(long int a, long int b)
{
	return a + b;
}

SYSCALL_DEFINE2(potato_plus, long int, a, long int , b)
{
	return sys_potato_plus(a, b);
}

asmlinkage long int sys_potato_minus(long int a, long int b)
{
	return a - b;
}

SYSCALL_DEFINE2(potato_minus, long int, a, long int , b)
{
	return sys_potato_minus(a, b);
}

asmlinkage long int sys_potato_multi(long int a, long int b)
{
	return a * b;
}

SYSCALL_DEFINE2(potato_multi, long int, a, long int , b)
{
	return sys_potato_multi(a, b);
}

asmlinkage long int sys_potato_mod(long int a, long int b)
{
	return a % b;
}

SYSCALL_DEFINE2(potato_mod, long int, a, long int , b)
{
	return sys_potato_mod(a, b);
}