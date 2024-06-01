#include "os_cfg.h"
    .text
    .extern kernel_init
    .extern init_main
    .global _start
    # void start (boot_info_t *boot_info)
_start:
    push %ebp
    mov %esp, %ebp
    mov 0x8(%ebp), %eax

    # mov 4(%esp), %eax
    # kernel_init(boot_info)
    push %eax
    call kernel_init
    jmp $KERNEL_SELECTOR_CS, $gdt_reload # 重新加载GDT
gdt_reload:
    mov $KERNEL_SELECTOR_DS, %ax
	mov %ax, %ds
	mov %ax, %ss
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

    ; mov $stack, %eax
    ; mov $KERNEL_STACK_SIZE, %ebx

    mov $(stack + KERNEL_STACK_SIZE), %esp
    jmp init_main

    .bss
.comm stack, KERNEL_STACK_SIZE # comm 声明未初始化的通用内存区域，以字节计

    .text
.macro exception_handler name num with_err_code

    .extern do_handler_\name
    .global exception_handler_\name
exception_handler_\name:
    .if \with_err_code == 0
        push $0
    .endif
    push $\num
    pusha
    push %ds
    push %es
    push %fs
    push %gs

    push %esp
    call do_handler_\name
    add $(1*4), %esp

    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa
    add $(2*4), %esp

    iret
.endm

exception_handler unknown, -1, 0
exception_handler divider, 0, 0
exception_handler Debug, 1, 0
exception_handler NMI, 2, 0
exception_handler breakpoint, 3, 0
exception_handler overflow, 4, 0
exception_handler bound_range, 5, 0
exception_handler invalid_opcode, 6, 0
exception_handler device_unavailable, 7, 0
exception_handler double_fault, 8, 1
exception_handler invalid_tss, 10, 1
exception_handler segment_not_present, 11, 1
exception_handler stack_segment_fault, 12, 1
exception_handler general_protection, 13, 1
exception_handler page_fault, 14, 1
exception_handler fpu_error, 16, 0
exception_handler alignment_check, 17, 1
exception_handler machine_check, 18, 0
exception_handler smd_exception, 19, 0
exception_handler virtual_exception, 20, 0

# 硬件中断
exception_handler timer, 0x20, 0

// eax, ecx, edx由调用者自动保存
// ebx, esi, edi, ebp需要由被调用者保存和恢复
// cs/ds/es/fs/gs/ss不用保存，因为都是相同的
// esp不用保存，只需要让esp保存在调用之前的状态
// eflags ?
    .text
    .global simple_switch
simple_switch:
    movl 4(%esp), %eax   // 取from->stack
	movl 8(%esp), %edx   // 取to->stack

	// 保存前一任务的状态
	push %ebp
	push %ebx
	push %esi
	push %edi

	// 切换栈
	mov %esp, (%eax)    // from->stack = esp
  	mov %edx, %esp      // esp = to->stack

	// 加载下一任务的栈
	pop %edi
	pop %esi
	pop %ebx
	pop %ebp
  	ret