SECTIONS {
    PROVIDE(kernel_base = 0x0);
    . = 0x10000;

    PROVIDE(s_text = .);
    .text : {
        *(EXCLUDE_FILE(*first_task*) .text)
    }

    .rodata : {
        *(EXCLUDE_FILE(*first_task*) .text)
    }
    PROVIDE(e_text = .);
    . = ALIGN(4096);
	PROVIDE(s_data = .);
    .data : {
        *(EXCLUDE_FILE(*first_task*) .text)
    }

    .bss : {
        *(EXCLUDE_FILE(*first_task*) .text)
    }
    PROVIDE(e_data = .);
    . = 0x80000000;
    PROVIDE(s_first_task = LOADADDR(.first_task));
    .first_task : AT(e_data) {
		*first_task_entry*(.text .data. rodata .data)
		*first_task*(.text .data. rodata .data)
	}
    PROVIDE(e_first_task = LOADADDR(.first_task) + SIZEOF(.first_task));

	PROVIDE(mem_free_start = e_first_task);
}