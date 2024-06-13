#include "fs/fs.h"
#include "comm/types.h"
#include "tools/klib.h"
#include "loader/loader.h"
#include "tools/log.h"
#include "dev/console.h"

#define FS_TABLE_SIZE 10 // 文件系统表数量
static uint8_t TEMP_ADDR[100 * 1024];
static uint8_t *temp_pos;
#define TEMP_FILE_ID 100

static void read_disk(int sector, int sector_count, uint8_t *buf)
{
    outb(0x1F6, (uint8_t)(0xE0)); // 选择磁盘
    outb(0x1F2, (uint8_t)(sector_count >> 8));
    outb(0x1F3, (uint8_t)(sector >> 24));
    outb(0x1F4, (uint8_t)(0));
    outb(0x1F5, (uint8_t)(0));

    outb(0x1F2, (uint8_t)(sector_count));
    outb(0x1F3, (uint8_t)(sector));
    outb(0x1F4, (uint8_t)(sector >> 8));
    outb(0x1F5, (uint8_t)(sector >> 16));

    outb(0x1F7, (uint8_t)(0x24));

    uint16_t *data_buf = (uint16_t *)buf;
    while (sector_count-- > 0)
    {
        while ((inb(0x1F7) & 0x88) != 0x8)
        {
            /* code */
        }
        for (int i = 0; i < SECTOR_SIZE / 2; i++)
        {
            *data_buf++ = inw(0x1F0);
            /* code */
        }
    }
}

int sys_open(const char *name, int flags, ...)
{
    if (name[0] == '/')
    {
        read_disk(5000, 80, (uint8_t *)TEMP_ADDR);
        temp_pos = (uint8_t *)TEMP_ADDR;
        return TEMP_FILE_ID;
    }
    return -1;
}

int sys_read(int file, char *ptr, int len)
{
    if (file == TEMP_FILE_ID)
    {
        kernel_memcpy(ptr, temp_pos, len);
        temp_pos += len;
        return len;
    }
    return -1;
}

int sys_write(int file, char *ptr, int len)
{
    if (file == 1)
    {
        // ptr[len] = '\0';
        console_write(0, ptr, len);
        // log_printf("%s", ptr);
    }
    return -1;
}

int sys_lseek(int file, int ptr, int dir)
{
    if (file == TEMP_FILE_ID)
    {
        temp_pos = (uint8_t *)(TEMP_ADDR + ptr);
        return 0;
    }

    return -1;
}

int sys_close(int file)
{
    return 0;
}

/**
 * 判断文件描述符与tty关联
 */
int sys_isatty(int file)
{
    return 0;
}

/**
 * @brief 获取文件状态
 */
int sys_fstat(int file, struct stat *st)
{
    return 0;
}