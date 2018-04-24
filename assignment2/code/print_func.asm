section .bss
    iobuf resb 1

section .text
global my_putchar

my_putchar:
    mov eax, 4
    mov ebx, 1
    mov ecx, [esp + 4]
    mov byte[iobuf], cl 
    mov ecx, iobuf
    mov edx, 1
    int 80h
    ret
