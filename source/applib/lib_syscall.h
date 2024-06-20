#ifndef LIB_SYSCALL_H
#define LIB_SYSCALL_H
#include "core/syscall.h"
#include "os_cfg.h"
#include "lib_syscall.h"
#include <sys/stat.h>

typedef struct _syscall_args_t
{
    int id;
    int arg0;
    int arg1;
    int arg2;
    int arg3;
} syscall_args_t;

int msleep(int ms);
int fork(void);
int getpid(void);
int yield(void);
int execve(const char *name, char *const *argv, char *const *env);
void print_msg(const char *fmt, int arg);

int open(const char *name, int flags, ...);
int read(int file, char *ptr, int len);
int write(int file, char *ptr, int len);
int close(int file);
int lseek(int file, int ptr, int dir);

int isatty(int file);
int fstat(int file, struct stat *st);
void *sbrk(ptrdiff_t incr);

int dup(int file);
void _exit(int status);
int wait(int *status);

struct dirent
{
    int index;      // 在目录中的偏移
    int type;       // 文件或目录的类型
    char name[255]; // 目录或目录的名称
    int size;       // 文件大小
};

typedef struct _DIR
{
    int index; // 当前遍历的索引
    struct dirent dirent;
} DIR;

DIR *opendir(const char *name);
struct dirent *readdir(DIR *dir);
int closedir(DIR *dir);
int unlink(const char *pathname);

int ioctl(int fd, int cmd, int arg0, int arg1);
#endif // LIB_SYSCALL_H