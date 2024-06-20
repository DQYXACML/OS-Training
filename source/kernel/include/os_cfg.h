#ifndef OS_CFG_H
#define OS_CFG_H

#define GDT_TABLE_SIZE 256
#define KERNEL_SELECTOR_CS (1 * 8)   // 内核代码段描述符
#define KERNEL_SELECTOR_DS (2 * 8)   // 内核数据段描述符
#define KERNEL_STACK_SIZE (8 * 1024) // 内核栈
#define SELECTOR_SYSCALL (3 * 8)     // 调用门的选择子
#define OS_TICK_MS 10                // 每毫秒的时钟数
#define OS_VERSION "1.0.0"
#define TASK_NR 128
#define ROOT_DEV DEV_DISK, 0xb1
#endif