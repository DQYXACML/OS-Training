#include <stdarg.h>
#include "tools/log.h"
#include "tools/klib.h"
#include "comm/cpu_instr.h"
#include "cpu/irq.h"
#include "ipc/mutex.h"

// 目标用串口，参考资料：https://wiki.osdev.org/Serial_Ports
#define LOG_USE_COM 0
#define COM1_PORT 0x3F8 // RS232端口0初始化

static mutex_t mutex;

void log_init(void)
{
    outb(COM1_PORT + 1, 0x00); // Disable all interrupts
    outb(COM1_PORT + 3, 0x80); // Enable DLAB (set baud rate divisor)
    outb(COM1_PORT + 0, 0x03); // Set divisor to 3 (lo byte) 38400 baud
    outb(COM1_PORT + 1, 0x00); //                  (hi byte)
    outb(COM1_PORT + 3, 0x03); // 8 bits, no parity, one stop bit
    outb(COM1_PORT + 2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
    mutex_init(&mutex);
}

void log_printf(const char *fmt, ...)
{
    char str_buf[128];
    va_list args;
    kernel_memset(str_buf, '\0', sizeof(str_buf));
    va_start(args, fmt);
    kernel_vsprintf(str_buf, fmt, args);
    va_end(args);

    // 临界区进入
    // irq_state_t state = irq_enter_protection();
    mutex_lock(&mutex);
    const char *p = str_buf;
    while (*p != '\0')
    {
        // 检查串口是否忙
        while ((inb(COM1_PORT + 5) & (1 << 6)) == 0)
        {
        }
        outb(COM1_PORT, *p++);
    }
    // 增加默认换行
    outb(COM1_PORT, '\r'); // 行号不变，列号归零
    outb(COM1_PORT, '\n'); // 列号不变，行号+1
    // irq_leave_protection(state); // 临界区退出
    mutex_unlock(&mutex);
}