#ifndef TASK_H
#define TASK_H
#include "comm/types.h"
#include "cpu/cpu.h"
#include "tools/list.h"
#include "fs/file.h"

#define TASK_NAME_SIZE 32          // 任务名字长度
#define TASK_TIME_SLICE_DEFAULT 10 // 时间片计数
#define TASK_FLAG_SYSTEM (1 << 0)  // 系统任务
#define TASK_OFILE_NR 128          // 最多支持打开的文件数量

typedef struct _task_t
{
    enum
    {
        TASK_CREATED,
        TASK_RUNNING,
        TASK_SLEEP,
        TASK_READY,
        TASK_WAITING,
        TASK_ZOMBIE,
    } state; // task状态

    char name[TASK_NAME_SIZE]; // 任务名字

    int pid;
    struct _task_t *parent; // 父进程

    uint32_t heap_start; // 堆的顶层地址
    uint32_t heap_end;   // 堆结束地址

    int sleep_ticks; // CPU 睡眠时间片
    int time_ticks;  // CPU 时间片
    int slice_ticks; // 递减时间片计数

    int status;

    list_node_t run_node; // 插入ready_list;
    list_node_t wait_node;
    list_node_t all_node; // 插入task_list;

    file_t *file_table[TASK_OFILE_NR]; // 任务最多打开的文件数量

    tss_t tss;
    int tss_sel; // tss 选择子
} task_t;

int task_init(task_t *task, const char *name, int flag, uint32_t entry, uint32_t esp);
void task_switch_from_to(task_t *from, task_t *to);
void simple_switch(uint32_t **from, uint32_t *to);

file_t *task_file(int fd);
int task_alloc_fd(file_t *file);
void task_remove_fd(int fd);

typedef struct _task_manager_t
{
    task_t *curr_task; // 当前运行的任务

    list_t ready_list; // 就绪队列
    list_t task_list;  // 所有已创建任务的队列
    list_t sleep_list; // 延时队列

    task_t first_task; // 内核任务
    task_t idle_task;

    int app_code_sel;
    int app_data_sel;
} task_manager_t;

void task_manager_init(void);
void task_first_init(void);
task_t *task_first_task(void);

void task_set_ready(task_t *task);
void task_set_block(task_t *task);
int sys_sched_yield(void);
void task_dispatch(void);
void task_time_tick(void); // 时钟终端处理函数

// 系统调用
void sys_msleep(uint32_t ms);
int sys_getpid(void);
int sys_fork(void);
int sys_execve(char *name, char **argv, char **env);
int sys_yield(void);
void sys_exit(int status);
int sys_wait(int *status);

void task_set_sleep(task_t *task, uint32_t ticks);
void task_set_wakeup(task_t *task);

task_t *task_current(void);

typedef struct _task_args_t
{
    uint32_t ret_addr; // 返回地址
    uint32_t argc;
    char **argv;
} task_args_t;

#endif