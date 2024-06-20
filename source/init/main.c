#include <stdio.h>
#include <string.h>
#include "lib_syscall.h"
#include "main.h"
#include <getopt.h>
#include <stdlib.h>
#include <sys/file.h>

int main(int argc, char **argv)
{
    int a = 3 / 0;
    *(char *)0 = 0x1234;

    return 0;
}