#include "core/task.h"
#include "tools/klib.h"
#include "os_cfg.h"
#include "cpu/cpu.h"
#include "tools/log.h"
#include "comm/cpu_instr.h"
#include "cpu/irq.h"
#include "cpu/mmu.h"
#include "core/memory.h"

static uint32_t idle_task_stack[1024];
static task_manager_t task_manager; // 任务管理器

static int tss_init(task_t *task, int flag, uint32_t entry, uint32_t esp)
{
    int tss_sel = get_alloc_desc();
    if (tss_sel < 0)
    {
        log_printf("alloc tss failed.\n");
        return -1;
    }

    segment_desc_set(tss_sel, (uint32_t)&task->tss, sizeof(tss_t), SEG_P_PRESENT | SEG_DPL0 | SEG_TYPE_TSS); // 设置GDT表项内容

    // tss段初始化
    kernel_memset(&task->tss, 0, sizeof(tss_t));

    // 分配内核栈，得到的是物理地址
    uint32_t kernel_stack = memory_alloc_page();
    if (kernel_stack == 0)
    {
        goto tss_init_failed;
    }

    int code_sel, data_sel;
    // code_sel = task_manager.app_code_sel | SEG_CPL3;
    // data_sel = task_manager.app_data_sel | SEG_CPL3;
    // 根据不同的权限选择不同的访问选择子
    if (flag & TASK_FLAG_SYSTEM)
    {
        code_sel = KERNEL_SELECTOR_CS;
        data_sel = KERNEL_SELECTOR_DS;
    }
    else
    {
        // 注意加了RP3,不然将产生段保护错误
        code_sel = task_manager.app_code_sel | SEG_CPL3;
        data_sel = task_manager.app_data_sel | SEG_CPL3;
    }

    task->tss.eip = entry;
    // task->tss.esp = task->tss.esp0 = esp;
    task->tss.esp = esp ? esp : kernel_stack + MEM_PAGE_SIZE; // 未指定栈则用内核栈，即运行在特权级0的进程
    task->tss.esp0 = kernel_stack + MEM_PAGE_SIZE;
    task->tss.ss = data_sel;            // 特权3
    task->tss.ss0 = KERNEL_SELECTOR_DS; // 特权0
    task->tss.es = task->tss.ds = task->tss.fs = task->tss.gs = data_sel;
    task->tss.cs = code_sel;
    task->tss.eflags = EFLAGS_DEFAULT | EFLAGS_IF;

    // 页表初始化
    uint32_t page_dir = memory_create_uvm();
    if (page_dir == 0)
    {
        goto tss_init_failed;
    }
    task->tss.cr3 = page_dir;
    task->tss_sel = tss_sel;
    return 0;
tss_init_failed:
    gdt_free_sel(tss_sel);

    if (kernel_stack)
    {
        memory_free_page(kernel_stack);
    }
    return -1;
}

int task_init(task_t *task, const char *name, int flag, uint32_t entry, uint32_t esp)
{
    ASSERT(task != (task_t *)0);

    int err = tss_init(task, flag, entry, esp);
    if (err < 0)
    {
        log_printf("init task failed.\n");
        return err;
    }

    // 任务字段初始化
    kernel_strncpy(task->name, name, TASK_NAME_SIZE);
    task->state = TASK_CREATED;
    task->sleep_ticks = 0;
    task->time_ticks = TASK_TIME_SLICE_DEFAULT;
    task->slice_ticks = task->time_ticks;
    list_node_init(&task->all_node);
    list_node_init(&task->run_node);
    list_node_init(&task->wait_node);

    irq_state_t state = irq_enter_protection();
    task->pid = (uint32_t)task;
    task_set_ready(task);
    list_insert_last(&task_manager.task_list, &task->all_node); // 插入所有task进队尾
    irq_leave_protection(state);
    return 0;
    // uint32_t *pesp = (uint32_t *)esp;
    // if (pesp)
    // {
    //     *(--pesp) = entry; // ret的ip
    //     *(--pesp) = 0;
    //     *(--pesp) = 0;
    //     *(--pesp) = 0;
    //     *(--pesp) = 0;
    // }
}

void task_switch_from_to(task_t *from, task_t *to)
{
    switch_to_tss(to->tss_sel);
    // simple_switch(&from->stack, to->stack);
}

/**
 * @brief 启动任务
 */
void task_start(task_t *task)
{
    irq_state_t state = irq_enter_protection();
    task_set_ready(task);
    irq_leave_protection(state);
}

/**
 * @brief 初始进程的初始化
 * 没有采用从磁盘加载的方式，因为需要用到文件系统，并且最好是和kernel绑在一定，这样好加载
 * 当然，也可以采用将init的源文件和kernel的一起编译。此里要调整好kernel.lds，在其中
 * 将init加载地址设置成和内核一起的，运行地址设置成用户进程运行的高处。
 * 不过，考虑到init可能用到newlib库，如果与kernel合并编译，在lds中很难控制将newlib的
 * 代码与init进程的放在一起，有可能与kernel放在了一起。
 * 综上，最好是分离。
 */
void task_first_init(void)
{
    void first_task_entry(void);
    // 以下获得的是bin文件在内存中的物理地址，起始和结束
    extern uint8_t s_first_task[], e_first_task[];
    // 分配的空间比实际存储的空间要大一些，多余的用于放置栈
    uint32_t copy_size = (uint32_t)(e_first_task - s_first_task);
    uint32_t alloc_size = 10 * MEM_PAGE_SIZE;
    ASSERT(copy_size < alloc_size);

    uint32_t first_start = (uint32_t)first_task_entry;
    task_init(&task_manager.first_task, "first task", 0, first_start, first_start + alloc_size);
    // 写TR寄存器，指示当前运行的第一个任务
    write_tr(task_manager.first_task.tss_sel);
    task_manager.curr_task = &task_manager.first_task;

    // 切换页表
    mmu_set_page_dir(task_manager.first_task.tss.cr3);

    // 分配一页内存供代码存放使用，然后将代码复制过去
    memory_alloc_page_for(first_start, alloc_size, PTE_P | PTE_W | PTE_U);
    kernel_memcpy((void *)first_start, (void *)&s_first_task, copy_size);

    // 启动进程
    // task_start(&task_manager.first_task);

    // 写TR寄存器，指示当前运行的第一个任务
    // write_tr(task_manager.first_task.tss_sel);
}

/**
 * @brief 返回初始任务
 */
task_t *task_first_task(void)
{
    return &task_manager.first_task;
}

static void idle_task_entry(void)
{
    for (;;)
    {
        hlt();
    }
}

void task_manager_init(void)
{
    int sel = get_alloc_desc();
    segment_desc_set(sel, 0x00000000, 0xFFFFFFFF,
                     SEG_P_PRESENT | SEG_DPL3 | SEG_S_NORMAL |
                         SEG_TYPE_DATA | SEG_TYPE_RW | SEG_D);
    task_manager.app_data_sel = sel;

    sel = get_alloc_desc();
    segment_desc_set(sel, 0x00000000, 0xFFFFFFFF,
                     SEG_P_PRESENT | SEG_DPL3 | SEG_S_NORMAL |
                         SEG_TYPE_CODE | SEG_TYPE_RW | SEG_D);
    task_manager.app_code_sel = sel;

    // 各队列初始化
    list_init(&task_manager.ready_list);
    list_init(&task_manager.task_list);
    list_init(&task_manager.sleep_list);
    task_manager.curr_task = (task_t *)0;

    task_init(&task_manager.idle_task, "idle_task", TASK_FLAG_SYSTEM, (uint32_t)idle_task_entry, (uint32_t)idle_task_stack + 1024);
}

/**
 * @brief 将任务插入就绪队列
 */
void task_set_ready(task_t *task)
{
    if (task == &task_manager.idle_task)
    {
        return;
    }
    list_insert_last(&task_manager.ready_list, &task->run_node);
    task->state = TASK_READY;
}

/**
 * @brief 将任务从就绪队列移除
 */
void task_set_block(task_t *task)
{
    if (task == &task_manager.idle_task)
    {
        return;
    }
    list_remove(&task_manager.ready_list, &task->run_node);
}

/**
 * @brief 获取当前正在运行的任务
 */
task_t *task_current(void)
{
    return task_manager.curr_task;
}

int sys_sched_yield(void)
{
    irq_state_t state = irq_enter_protection();
    if (list_count(&task_manager.ready_list) > 1)
    {
        task_t *curr_task = task_current();

        // 如果队列中还有其它任务，则将当前任务移入到队列尾部
        task_set_block(curr_task);
        task_set_ready(curr_task);

        // 切换至下一个任务，在切换完成前要保护，不然可能下一任务
        // 由于某些原因运行后阻塞或删除，再回到这里切换将发生问题
        task_dispatch();
    }
    irq_leave_protection(state);
    return 0;
}

/**
 * @brief 获取下一将要运行的任务
 */
static task_t *task_next_run(void)
{
    if (list_count(&task_manager.ready_list) == 0)
    {
        return &task_manager.idle_task;
    }

    // 普通任务
    list_node_t *task_node = list_first(&task_manager.ready_list);
    return list_node_parent(task_node, task_t, run_node);
}

/**
 * @brief 进行一次任务调度
 */
void task_dispatch(void)
{
    irq_state_t state = irq_enter_protection();
    task_t *to = task_next_run();
    if (to != task_manager.curr_task)
    {
        task_t *from = task_manager.curr_task;

        task_manager.curr_task = to;
        to->state = TASK_RUNNING;
        task_switch_from_to(from, to);
    }
    irq_leave_protection(state);
}

/**
 * @brief 时间处理
 * 该函数在中断处理函数中调用
 */
void task_time_tick(void)
{
    task_t *curr_task = task_current();
    irq_state_t state = irq_enter_protection();
    if (--curr_task->slice_ticks == 0)
    {
        // 时间片用完，重新加载时间片
        // 对于空闲任务，此处减未用
        curr_task->slice_ticks = curr_task->time_ticks;

        // 调整队列的位置到尾部，不用直接操作队列
        task_set_block(curr_task);
        task_set_ready(curr_task);
    }

    // 睡眠处理
    list_node_t *curr = list_first(&task_manager.sleep_list);
    while (curr)
    {
        list_node_t *next = list_node_next(curr);

        task_t *task = list_node_parent(curr, task_t, run_node);
        if (--task->sleep_ticks == 0)
        {
            // 延时时间到达，从睡眠队列中移除，送至就绪队列
            task_set_wakeup(task);
            task_set_ready(task);
        }
        curr = next;
    }
    task_dispatch();
    irq_leave_protection(state);
}

/**
 * @brief 将任务加入睡眠状态
 */
void task_set_sleep(task_t *task, uint32_t ticks)
{
    if (ticks <= 0)
    {
        return;
    }

    task->sleep_ticks = ticks;
    task->state = TASK_SLEEP;
    list_insert_last(&task_manager.sleep_list, &task->run_node);
}

/**
 * @brief 将任务从延时队列移除
 *
 * @param task
 */
void task_set_wakeup(task_t *task)
{
    list_remove(&task_manager.sleep_list, &task->run_node);
}

/**
 * @brief 任务进入睡眠状态
 *
 * @param ms
 */
void sys_msleep(uint32_t ms)
{
    // 至少延时1个tick
    if (ms < OS_TICK_MS)
    {
        ms = OS_TICK_MS;
    }

    irq_state_t state = irq_enter_protection();

    // 从就绪队列移除，加入睡眠队列
    task_set_block(task_manager.curr_task);
    task_set_sleep(task_manager.curr_task, (ms + (OS_TICK_MS - 1)) / OS_TICK_MS);

    // 进行一次调度
    task_dispatch();

    irq_leave_protection(state);
}

/**
 * 返回任务的pid
 */
int sys_getpid(void)
{
    task_t *curr_task = task_current();
    return curr_task->pid;
}