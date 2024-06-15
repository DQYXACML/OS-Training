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
#include "core/memory.h"
#include "dev/console.h"
#include "dev/kbd.h"
#include "fs/fs.h"

void kernel_init(boot_info_t *boot_info)
{
    cpu_init();
    irq_init();
    log_init();

    // 内存初始化要放前面一点，因为后面的代码可能需要内存分配
    memory_init(boot_info);
    fs_init();

    time_init();

    task_manager_init();
}

// static task_t init_task;
// static uint32_t init_task_stack[1024];
// static sem_t sem;
// static task_t first_task;

// void init_task_entry(void)
// {
//     int count = 0;
//     for (;;)
//     {
//         sem_wait(&sem);
//         log_printf("init_task_entry: %d", count++);
//         // sys_sched_yield();
//         // task_switch_from_to(&init_task, task_first_task());
//     }
// }

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

/**
 * @brief 移至第一个进程运行
 */
void move_to_first_task(void)
{
    // 不能直接用Jmp far进入，因为当前特权级0，不能跳到低特权级的代码
    // 下面的iret后，还需要手动加载ds, fs, es等寄存器值，iret不会自动加载
    // 注意，运行下面的代码可能会产生异常：段保护异常或页保护异常。
    // 可根据产生的异常类型和错误码，并结合手册来找到问题所在
    task_t *curr = task_current();
    ASSERT(curr != 0);

    tss_t *tss = &(curr->tss);

    // 也可以使用类似boot跳loader中的函数指针跳转
    // 这里用jmp是因为后续需要使用内联汇编添加其它代码
    __asm__ __volatile__(
        // 模拟中断返回，切换入第1个可运行应用进程
        // 不过这里并不直接进入到进程的入口，而是先设置好段寄存器，再跳过去
        "push %[ss]\n\t"     // SS
        "push %[esp]\n\t"    // ESP
        "push %[eflags]\n\t" // EFLAGS
        "push %[cs]\n\t"     // CS
        "push %[eip]\n\t"    // ip
        "iret\n\t" ::[ss] "r"(tss->ss),
        [esp] "r"(tss->esp), [eflags] "r"(tss->eflags),
        [cs] "r"(tss->cs), [eip] "r"(tss->eip));
}

void init_main(void)
{
    // list_test();

    log_printf("Kernel is running...");
    log_printf("Version: %s", OS_VERSION);
    log_printf("%d %d %x %c", 123456, -123, 0x123456, 'a');

    // task_init(&init_task, "init task", (uint32_t)init_task_entry, (uint32_t)&init_task_stack[1024]); // 压栈先-4再入栈
    task_first_init();
    move_to_first_task();

    // sem_init(&sem, 0);
    // irq_enable_global(); // 用时间片中断来调度

    // int count = 0;
    // for (;;)
    // {
    //     log_printf("init_main: %d", count++);
    //     sem_notify(&sem);
    //     // sys_msleep(1000);
    //     // task_switch_from_to(task_first_task(), &init_task);
    //     // sys_sched_yield();
    // }
}