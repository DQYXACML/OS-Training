#include "fs/fs.h"
#include "comm/types.h"
#include "tools/klib.h"
#include "loader/loader.h"
#include "tools/log.h"
#include "dev/console.h"
#include "fs/file.h"
#include "dev/dev.h"

#define FS_TABLE_SIZE 10 // 文件系统表数量
static uint8_t TEMP_ADDR[100 * 1024];
static uint8_t *temp_pos;
#define TEMP_FILE_ID 100

/**
 * @brief 判断文件描述符是否正确
 */
static int is_fd_bad(int file)
{
    if ((file < 0) && (file >= TASK_OFILE_NR))
    {
        return 1;
    }

    return 0;
}

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

static int is_path_valid(const char *path)
{
    if ((path == (const char *)0) || (path[0] == '\0'))
    {
        return 0;
    }
    return 1;
}

int sys_open(const char *name, int flags, ...)
{
    if (kernel_strncmp(name + 5, "tty", 3) == 0)
    {
        if (!is_path_valid(name))
        {
            return -1;
        }
        // 分配文件描述符链接
        file_t *file = file_alloc();
        if (!file)
        {
            return -1;
        }
        int fd = task_alloc_fd(file);
        if (fd < 0)
        {
            goto sys_open_failed;
        }
        if (kernel_strlen(name) < 5)
        {
            goto sys_open_failed;
        }
        int num = name[8] - '0';
        int dev_id = dev_open(DEV_TTY, num, 0);
        if (dev_id < 0)
        {
            goto sys_open_failed;
        }
        file->dev_id = dev_id;
        file->mode = 0;
        file->pos = 0;
        file->ref = 1;
        file->type = FILE_TTY;
        kernel_strncpy(file->file_name, name, FILE_NAME_SIZE);
        return fd;
    sys_open_failed:
        if (file)
        {
            file_free(file);
        }

        if (fd >= 0)
        {
            task_remove_fd(fd);
        }
        return -1;
    }
    else
    {
        if (name[0] == '/')
        {
            read_disk(5000, 80, (uint8_t *)TEMP_ADDR);
            temp_pos = (uint8_t *)TEMP_ADDR;
            return TEMP_FILE_ID;
        }
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
    else
    {
        file = 0;
        file_t *p_file = task_file(file);
        if (!p_file)
        {
            log_printf("file not opened");
            return -1;
        }
        return dev_read(p_file->dev_id, 0, ptr, len);
    }
    return -1;
}

int sys_write(int file, char *ptr, int len)
{
    // file = 0; // 临时, dup实现后取消
    file_t *p_file = task_file(file);
    if (!p_file)
    {
        log_printf("file not opened");
        return -1;
    }
    return dev_write(p_file->dev_id, 0, ptr, len);
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

/**
 * @brief 文件系统初始化
 */
void fs_init(void)
{
    // mount_list_init();
    file_table_init();

    // // 磁盘检查
    // disk_init();

    // // 挂载设备文件系统，待后续完成。挂载点名称可随意
    // fs_t *fs = mount(FS_DEVFS, "/dev", 0, 0);
    // ASSERT(fs != (fs_t *)0);

    // // 挂载根文件系统
    // root_fs = mount(FS_FAT16, "/home", ROOT_DEV);
    // ASSERT(root_fs != (fs_t *)0);
}

/**
 * 复制一个文件描述符
 */
int sys_dup(int file)
{
    // 超出进程所能打开的全部，退出
    if (is_fd_bad(file))
    {
        log_printf("file(%d) is not valid.", file);
        return -1;
    }

    file_t *p_file = task_file(file);
    if (!p_file)
    {
        log_printf("file not opened");
        return -1;
    }

    int fd = task_alloc_fd(p_file); // 新fd指向同一描述符
    if (fd >= 0)
    {
        file_inc_ref(p_file);
        return fd;
    }

    log_printf("No task file avaliable");
    return -1;
}
