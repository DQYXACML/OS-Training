#ifndef LIB_SYSCALL_H
#define LIB_SYSCALL_H
#include "core/syscall.h"
#include "os_cfg.h"
#include "lib_syscall.h"

typedef struct _syscall_args_t
{
    int id;
    int arg0;
    int arg1;
    int arg2;
    int arg3;
} syscall_args_t;

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

static inline void msleep(int ms)
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

static inline int getpid(void)
{
    syscall_args_t args;
    args.id = SYS_getpid;
    return sys_call(&args);
}

static inline void print_msg(const char *fmt, int arg)
{
    syscall_args_t args;
    args.id = SYS_printmsg;
    args.arg0 = (int)fmt;
    args.arg1 = arg;
    sys_call(&args);
}

#endif // LIB_SYSCALL_H