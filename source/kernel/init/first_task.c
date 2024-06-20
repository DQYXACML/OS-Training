#include "tools/log.h"
#include "core/task.h"
#include "applib/lib_syscall.h"
#include "dev/tty.h"

int first_task_main(void)
{
#if 0
    int count = 3;

    int pid = getpid();
    pid = fork();
    print_msg("First task id=%d", pid);
    if (pid < 0)
    {
        print_msg("create task failed\n", 0);
    }
    else if (pid == 0)
    {
        print_msg("child: %d\n", count);
        char *argv[] = {"arg0", "arg1", "arg2", "arg3"};
        execve("/shell.elf", argv, (char **)0);
    }
    else
    {
        print_msg("child task id=%d\n", pid);
        print_msg("parent: %d\n", count);
    }
#endif
    for (int i = 0; i < TTY_NR; i++)
    {
        int pid = fork();
        if (pid < 0)
        {
            print_msg("create shell proc failed", 0);
            break;
        }
        else if (pid == 0)
        {
            // 子进程
            char tty_num[] = "/dev/tty?";
            tty_num[sizeof(tty_num) - 2] = i + '0';
            char *argv[] = {tty_num, (char *)0};
            execve("shell.elf", argv, (char **)0);
            print_msg("create shell proc failed", 0);
            while (1)
            {
                msleep(10000);
            }
        }
    }

    for (;;)
    {
        // 不断收集孤儿进程
        int status;
        wait(&status);
    }
}