#include "core/syscall.h"
#include "os_cfg.h"
#include "lib_syscall.h"

/**
 * 执行系统调用
 */
static inline int sys_call(syscall_args_t *args)
{
    const unsigned long sys_gate_addr[] = {0, SELECTOR_SYSCALL | 0}; // 使用特权级0
    int ret;

    // 采用调用门, 这里只支持5个参数
    // 用调用门的好处是会自动将参数复制到内核栈中，这样内核代码很好取参数
    // 而如果采用寄存器传递，取参比较困难，需要先压栈再取
    __asm__ __volatile__(
        "push %[arg3]\n\t"
        "push %[arg2]\n\t"
        "push %[arg1]\n\t"
        "push %[arg0]\n\t"
        "push %[id]\n\t"
        "lcalll *(%[gate])\n\n"
        : "=a"(ret)
        : [arg3] "r"(args->arg3), [arg2] "r"(args->arg2), [arg1] "r"(args->arg1),
          [arg0] "r"(args->arg0), [id] "r"(args->id),
          [gate] "r"(sys_gate_addr));
    return ret;
}

int msleep(int ms)
{
    if (ms <= 0)
    {
        return;
    }

    syscall_args_t args;
    args.id = SYS_msleep;
    args.arg0 = ms;
    sys_call(&args);
}

int getpid(void)
{
    syscall_args_t args;
    args.id = SYS_getpid;
    return sys_call(&args);
}

void print_msg(const char *fmt, int arg)
{
    syscall_args_t args;
    args.id = SYS_printmsg;
    args.arg0 = (int)fmt;
    args.arg1 = arg;
    sys_call(&args);
}

int fork(void)
{
    syscall_args_t args;
    args.id = SYS_fork;
    return sys_call(&args);
}

int execve(const char *name, char *const *argv, char *const *env)
{
    syscall_args_t args;
    args.id = SYS_execve;
    args.arg0 = (int)name;
    args.arg1 = (int)argv;
    args.arg2 = (int)env;
    return sys_call(&args);
}

int yield(void)
{
    syscall_args_t args;
    args.id = SYS_yield;
    return sys_call(&args);
}

int open(const char *name, int flags, ...)
{
    // 不考虑支持太多参数
    syscall_args_t args;
    args.id = SYS_open;
    args.arg0 = (int)name;
    args.arg1 = (int)flags;
    return sys_call(&args);
}

int read(int file, char *ptr, int len)
{
    syscall_args_t args;
    args.id = SYS_read;
    args.arg0 = (int)file;
    args.arg1 = (int)ptr;
    args.arg2 = len;
    return sys_call(&args);
}

int write(int file, char *ptr, int len)
{
    syscall_args_t args;
    args.id = SYS_write;
    args.arg0 = (int)file;
    args.arg1 = (int)ptr;
    args.arg2 = len;
    return sys_call(&args);
}

int close(int file)
{
    syscall_args_t args;
    args.id = SYS_close;
    args.arg0 = (int)file;
    return sys_call(&args);
}

int lseek(int file, int ptr, int dir)
{
    syscall_args_t args;
    args.id = SYS_lseek;
    args.arg0 = (int)file;
    args.arg1 = (int)ptr;
    args.arg2 = dir;
    return sys_call(&args);
}

/**
 * 获取文件的状态
 */
int fstat(int file, struct stat *st)
{
    syscall_args_t args;
    args.id = SYS_fstat;
    args.arg0 = (int)file;
    args.arg1 = (int)st;
    return sys_call(&args);
}

/**
 * 判断文件描述符与tty关联
 */
int isatty(int file)
{
    syscall_args_t args;
    args.id = SYS_isatty;
    args.arg0 = (int)file;
    return sys_call(&args);
}

void *sbrk(ptrdiff_t incr)
{
    syscall_args_t args;
    args.id = SYS_sbrk;
    args.arg0 = (int)incr;
    return (void *)sys_call(&args);
}

int dup(int file)
{
    syscall_args_t args;
    args.id = SYS_dup;
    args.arg0 = file;
    return sys_call(&args);
}