#include "tools/log.h"
#include "core/task.h"
#include "applib/lib_syscall.h"

int first_task_main(void)
{
    int pid = getpid();
    for (;;)
    {
        print_msg("task id=%d", pid);
        // log_printf("first task.");
        // sys_msleep(1000);
        msleep(1000);
    }
}