/* boot.s - AT&T Syntax */

.set ALIGN,    1<<0
.set MEMINFO,  1<<1
.set FLAGS,    ALIGN | MEMINFO
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

.section .bss
.align 16
stack_bottom:
.skip 16384 # 16 KiB
stack_top:
.global multiboot_ptr
.lcomm multiboot_ptr, 4

.section .text
.global _start
.type _start, @function
_start:
    movl %ebx, multiboot_ptr
	movl $stack_top, %esp
	call kernel_main
	cli
1:	hlt
	jmp 1b
.size _start, . - _start

.global gdt_flush
.type gdt_flush, @function
gdt_flush:
    movl 4(%esp), %eax  /* Get the GDT pointer */
    lgdt (%eax)         /* Load the GDT pointer */
    
    movw $0x10, %ax     /* 0x10 is the offset of our data segment */
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    
    /* Jump to a new code segment to flush the CPU pipeline */
    /* 0x08 is the offset of our code segment */
    ljmp $0x08, $flush_cs
flush_cs:
    ret

/* --- INTERRUPT SERVICE ROUTINES (ISRs) --- */
/* This macro creates a stub for an ISR that does not have an error code */
.macro ISR_NOERR int_no
.global isr\int_no
isr\int_no:
    cli
	pushl $\int_no
    pushl $0            /* Push a dummy error code */
    jmp isr_common_stub
.endm

/* Common stub for all ISRs */
isr_common_stub:
    pusha               /* Pushes edi,esi,ebp,esp,ebx,edx,ecx,eax */
    
    movw %ds, %ax       /* Save the data segment descriptor */
    pushl %eax

    movw $0x10, %ax     /* Load the kernel data segment descriptor */
    movw %ax, %ds; movw %ax, %es; movw %ax, %fs; movw %ax, %gs

    /* THE CRITICAL FIX: Pass a pointer to the stack to the C handler */
    movl %esp, %eax     /* Get the current stack pointer */
    pushl %eax          /* Push it as the argument for our C function */

    call fault_handler  /* Call our C-level handler */

    addl $4, %esp       /* Clean up the pushed pointer */
    
    popl %eax           /* Restore the original data segment */
    movw %ax, %ds; movw %ax, %es; movw %ax, %fs; movw %ax, %gs;

    popa                /* Pop edi,esi,ebp,esp,ebx,edx,ecx,eax */
    addl $8, %esp       /* Cleans up the pushed error code and interrupt number */
    iret                /* Pop CS, EIP, EFLAGS, SS, and ESP */

/* Generate the first 32 ISR stubs */
ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
/* ISR 8 has an error code, but we'll treat it the same for simplicity */
ISR_NOERR 8 
ISR_NOERR 9
/* ISRs 10-14 have error codes */
ISR_NOERR 10
ISR_NOERR 11
ISR_NOERR 12
ISR_NOERR 13
ISR_NOERR 14
ISR_NOERR 15
ISR_NOERR 16
/* ISR 17 has an error code */
ISR_NOERR 17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
/* ... and so on up to 31 */
ISR_NOERR 21; ISR_NOERR 22; ISR_NOERR 23; ISR_NOERR 24; ISR_NOERR 25;
ISR_NOERR 26; ISR_NOERR 27; ISR_NOERR 28; ISR_NOERR 29; ISR_NOERR 30;
/* ISR 31 is reserved */
ISR_NOERR 31

.global trigger_int
.type trigger_int, @function
trigger_int:
    movl 4(%esp), %eax          /* Get interrupt number from the stack */
    jmp *int_jumptable(,%eax,4)  /* Jump to the address in the table at index EAX */

int_jumptable:
    .long int_stub_0; .long int_stub_1; .long int_stub_2; .long int_stub_3; .long int_stub_4; .long int_stub_5; .long int_stub_6; .long int_stub_7;
    .long int_stub_8; .long int_stub_9; .long int_stub_10; .long int_stub_11; .long int_stub_12; .long int_stub_13; .long int_stub_14; .long int_stub_15;
    .long int_stub_16; .long int_stub_17; .long int_stub_18; .long int_stub_19; .long int_stub_20; .long int_stub_21; .long int_stub_22; .long int_stub_23;
    .long int_stub_24; .long int_stub_25; .long int_stub_26; .long int_stub_27; .long int_stub_28; .long int_stub_29; .long int_stub_30; .long int_stub_31;

int_stub_0: int $0; int_stub_1: int $1; int_stub_2: int $2; int_stub_3: int $3; int_stub_4: int $4; int_stub_5: int $5; int_stub_6: int $6; int_stub_7: int $7;
int_stub_8: int $8; int_stub_9: int $9; int_stub_10: int $10; int_stub_11: int $11; int_stub_12: int $12; int_stub_13: int $13; int_stub_14: int $14; int_stub_15: int $15;
int_stub_16: int $16; int_stub_17: int $17; int_stub_18: int $18; int_stub_19: int $19; int_stub_20: int $20; int_stub_21: int $21; int_stub_22: int $22; int_stub_23: int $23;
int_stub_24: int $24; int_stub_25: int $25; int_stub_26: int $26; int_stub_27: int $27; int_stub_28: int $28; int_stub_29: int $29; int_stub_30: int $30; int_stub_31: int $31;

/* --- Other Assembly Functions --- */
.global inb
.type inb, @function
inb:
    movl 4(%esp), %edx
    inb %dx, %al
    ret

.global cpuid
.type cpuid, @function
cpuid:
    pushl %ebp; movl %esp, %ebp; pushl %ebx;
    movl 8(%ebp), %eax; cpuid; movl 12(%ebp), %edi; movl %eax, (%edi);
    movl 16(%ebp), %edi; movl %ebx, (%edi); movl 20(%ebp), %edi;
    movl %ecx, (%edi); movl 24(%ebp), %edi; movl %edx, (%edi);
    popl %ebx; movl %ebp, %esp; popl %ebp; ret

/* New function to load the IDT register */
.global idt_load
.type idt_load, @function
idt_load:
    movl 4(%esp), %eax
    lidt (%eax)
    ret

.global outw
.type outw, @function
outw:
    movl 4(%esp), %eax  /* Value to write */
    movl 8(%esp), %edx  /* Port to write to */
    outw %ax, %dx
    ret
