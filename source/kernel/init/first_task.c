#include "tools/log.h"
#include "core/task.h"
#include "applib/lib_syscall.h"

int first_task_main(void)
{
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

    for (;;)
    {
        print_msg("task id=%d", pid);
        // log_printf("first task.");
        // sys_msleep(1000);
        msleep(1000);
    }
}