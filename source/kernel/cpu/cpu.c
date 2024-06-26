#include "cpu/cpu.h"
#include "os_cfg.h"
#include "comm/cpu_instr.h"
#include "cpu/irq.h"
#include "ipc/mutex.h"
#include "core/syscall.h"

static segment_desc_t gdt_table[GDT_TABLE_SIZE];
static mutex_t mutex;

void segment_desc_set(int selector, uint32_t base, uint32_t limit, uint16_t attr)
{
    segment_desc_t *desc = gdt_table + (selector / sizeof(segment_desc_t)); //

    // 设置limit单位为4kB
    if (limit > 0xFFFF)
    {
        attr |= SEG_G; // G 标志位
        limit /= 0x1000;
    }

    desc->limit15_0 = limit & 0xFFFF;
    desc->base15_0 = base & 0xFFFF;
    desc->base23_16 = (base >> 16) & 0xFF;
    desc->base31_24 = (base >> 24) & 0xFF;
    desc->attr = attr | (((limit >> 16) & 0xFF) << 8);
}

void gdt_free_sel(int sel)
{
    mutex_lock(&mutex);
    gdt_table[sel / sizeof(segment_desc_t)].attr = 0;
    mutex_unlock(&mutex);
}

int get_alloc_desc()
{
    // irq_state_t state = irq_enter_protection();
    mutex_lock(&mutex);

    for (int i = 1; i < GDT_TABLE_SIZE; i++)
    {
        segment_desc_t *desc = gdt_table + i;
        if (desc->attr == 0) // 根据属性是否为0判断
        {
            // irq_leave_protection(state);
            mutex_unlock(&mutex);
            return i * sizeof(segment_desc_t);
        }
    }
    // irq_leave_protection(state);
    mutex_unlock(&mutex);
    return -1;
}

void init_gdt(void)
{
    // 全部清空
    for (int i = 0; i < GDT_TABLE_SIZE; i++)
    {
        segment_desc_set(i * sizeof(segment_desc_t), 0, 0, 0); // 找到第几个GDT表项
    }
    // 数据段
    segment_desc_set(KERNEL_SELECTOR_DS, 0, 0xFFFFFFFF,
                     SEG_P_PRESENT | SEG_DPL0 | SEG_S_NORMAL | SEG_TYPE_DATA | SEG_TYPE_RW | SEG_D);
    // 代码段
    segment_desc_set(KERNEL_SELECTOR_CS, 0, 0xFFFFFFFF,
                     SEG_P_PRESENT | SEG_DPL0 | SEG_S_NORMAL | SEG_TYPE_CODE | SEG_TYPE_RW | SEG_D);

    // 调用门
    gate_desc_set((gate_desc_t *)(gdt_table + (SELECTOR_SYSCALL >> 3)),
                  KERNEL_SELECTOR_CS,
                  (uint32_t)exception_handler_syscall,
                  GATE_P_PRESENT | GATE_DPL3 | GATE_TYPE_SYSCALL | SYSCALL_PARAM_COUNT);
    // 重新加载GDT
    lgdt((uint32_t)gdt_table, sizeof(gdt_table));
}

void gate_desc_set(gate_desc_t *desc, uint16_t selector, uint32_t offset, uint16_t attr)
{
    desc->offset15_0 = offset & 0xFFFF;
    desc->selector = selector;
    desc->attr = attr;
    desc->offset31_16 = (offset >> 16) & 0xFFFFF;
}

void cpu_init(void)
{
    mutex_init(&mutex);
    init_gdt();
}

void switch_to_tss(uint32_t tss_selector)
{
    far_jump(tss_selector, 0);
}