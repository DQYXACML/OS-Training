#include "init.h"
#include "comm/boot_info.h"
#include "cpu/cpu.h"
#include "cpu/irq.h"
#include "dev/time.h"
#include "tools/log.h"
#include "os_cfg.h"
#include "tools/klib.h"
#include "core/task.h"
#include "tools/list.h"
#include "comm/cpu_instr.h"
#include "ipc/sem.h"

void kernel_init(boot_info_t *boot_info)
{
    cpu_init();
    log_init();
    irq_init();
    time_init();

    task_manager_init();
}

static task_t init_task;
static uint32_t init_task_stack[1024];
static sem_t sem;
// static task_t first_task;

void init_task_entry(void)
{
    int count = 0;
    for (;;)
    {
        sem_wait(&sem);
        log_printf("init_task_entry: %d", count++);
        // sys_sched_yield();
        // task_switch_from_to(&init_task, task_first_task());
    }
}

// 链表测试
// void list_test(void)
// {
//     list_t list;

//     list_init(&list);
//     log_printf("list: first=0x%x, last=0x%x, count=%d", list_first(&list), list_last(&list), list_count(&list));

//     for (int i = 0; i < 5; i++)
//     {
//         list_node_t *node = node + i;

//         log_printf("insert first to list: %d, 0x%x", i, (uint32_t)node);
//         list_insert_first(&list, node);
//     }
//     log_printf("list: first=0x%x, last=0x%x, count=%d", list_first(&list), list_last(&list), list_count(&list));

//     list_init(&list);
//     for (int i = 0; i < 5; i++)
//     {
//         list_node_t *node = node + i;

//         log_printf("insert first to list: %d, 0x%x", i, (uint32_t)node);
//         list_insert_last(&list, node);
//     }
//     log_printf("list: first=0x%x, last=0x%x, count=%d", list_first(&list), list_last(&list), list_count(&list));
// }

void init_main(void)
{
    // list_test();

    log_printf("Kernel is running...");
    log_printf("Version: %s", OS_VERSION);
    log_printf("%d %d %x %c", 123456, -123, 0x123456, 'a');

    task_init(&init_task, "init task", (uint32_t)init_task_entry, (uint32_t)&init_task_stack[1024]); // 压栈先-4再入栈
    task_first_init();

    sem_init(&sem, 0);
    irq_enable_global(); // 用时间片中断来调度

    int count = 0;
    for (;;)
    {
        log_printf("init_main: %d", count++);
        sem_notify(&sem);
        sys_msleep(1000);
        // task_switch_from_to(task_first_task(), &init_task);
        // sys_sched_yield();
    }
}