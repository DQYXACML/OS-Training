#include "cpu/irq.h"
#include "dev/dev.h"
#include "tools/klib.h"

#define DEV_TABLE_SIZE 128 // 支持的设备数量

extern dev_desc_t dev_tty_desc;
extern dev_desc_t dev_disk_desc;

// 设备描述表
static dev_desc_t *dev_desc_tbl[] = {
    &dev_tty_desc,
    &dev_disk_desc,
};

// 设备表
static device_t dev_tbl[DEV_TABLE_SIZE];
