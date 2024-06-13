#include "lib_syscall.h"
#include <stdio.h>

int main(int argc, char **argv)
{
    sbrk(0);
    sbrk(100);
    sbrk(200);
    sbrk(4096 * 2 + 200);
    sbrk(4096 * 5 + 1234);

    printf("abef\b\b\b\bcd\n");
    printf("abcd\x7f;fg\n");
    printf("\0337Hello, world!\0338123\n");
    printf("\033 [31;42mHello,world!\033[39;49m123");

    printf("123\033[2DHello,world!\n"); // 光标左移2，1Hello,world!
    printf("123\033[2CHello,world!\n"); // 光标右移2，123  Hello,wrold!

    printf("\033[31m");
    printf("\033[10;10H test!\n");
    printf("\033[20;20H test!\n");
    printf("\033[32;25;39m123\n");

    printf("\033[2J");

    printf("Hello from shell\n");
    for (int i = 0; i < argc; i++)
    {
        printf("arg: %s\n", argv[i]);
    }

    fork();
    yield();

    for (;;)
    {
        printf("shell pid=%d\n", getpid());
        msleep(1000);
    }
}