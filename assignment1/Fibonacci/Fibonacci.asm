%define size 8192

section .data
	prompt db 'Please input x and y: '
	len_prompt equ $-prompt
	end_line db 0Ah
	green db 1Bh, '[32;1m', 0
	.len equ $-green
	yellow db 1Bh, '[33;1m', 0
	.len equ $-yellow
	blue db 1Bh, '[34;1m', 0
	.len equ $-blue
	purple db 1Bh, '[35;1m', 0
	.len equ $-purple

section .bss
	iobuf resb 1
	a resb size
	b resb size

section .text
global main

main:
	;show prompt
	mov rax, 1
	mov rdi, 1
	mov rsi, prompt
	mov rdx, len_prompt
	syscall
	
	;read first number
	call getNum
	push rax ;parameter passing
	
	;read second number
	call getNum
	
	mov rsi, rax ;move end to rsi
	pop rdi ;move begin to rdi
	
	call calculate ;calculate and show results

	mov rax, 0
	ret
	
calculate: ; void calculate(int begin, int end)
	;calculate and show
	;including begin and end
	;begin in rdi, end in rsi
	mov rax, 0
clean:
	;clean memory of a and b
	mov byte[a + rax], 0
	mov byte[b + rax], 0
	inc rax
	cmp rax, size
	jl clean
init:
	;do some initialization
	mov r15, 0  ;carry
	mov rax, -1 ;count
	mov byte[b], 1
loop:
	inc rax
	cmp rax, rdi
	jl begin_add
	;rax greater than rsi means that we have finished the calculation
	cmp rax, rsi
	jg end
	;between begin and end -> print number
	push rdi
	mov rdi, rax ;pass the index of the number to print function
	push rax
	call print
	pop rax
	pop rdi
begin_add:
	mov rcx, 0 ;clean digit count
digit_add:
	movzx r8, byte[a + rcx]
	movzx r9, byte[b + rcx]
	mov r10, r8
	add r10, r9
	add r10, r15 ;add carry
	mov r15, 0 ;clear carry
	cmp r10, 10
	jl save
	mov r15, 1
	sub r10, 10
save:
	mov byte[a + rcx], r9b
	mov byte[b + rcx], r10b
	inc rcx
	cmp rcx, size
	jl digit_add
	jmp loop
end:
	ret


print: 
	;void print(int num), num here means the index of fib
	;I will choose different color according to the index
	push rdx
	push rsi
set_color:
	and rdi, 3
	cmp rdi, 0
	je case_0
	cmp rdi, 1
	je case_1
	cmp rdi, 2
	je case_2
	cmp rdi, 3
	je case_3
case_0:
	mov rsi, green
	mov rdx, green.len
	jmp general
case_1:
	mov rsi, yellow
	mov rdx, yellow.len
	jmp general
case_2:
	mov rsi, blue
	mov rdx, blue.len
	jmp general
case_3:
	mov rsi, purple
	mov rdx, purple.len
	jmp general
general:	
	mov rax, 1
	mov rdi, 1
	syscall
cal_length:
	;calculate the length of number
	mov rsi, a + size ;counter
length_loop:	
	dec rsi
	cmp byte[rsi], 0
	jne print_digit
	cmp rsi, a ;if num is zero
	je print_digit 
	jmp length_loop
print_digit:	
	cmp rsi, a
	jl  print_end
	mov dl, byte[rsi] ;dl is the last part of rdx
	add dl, 48
	mov byte[iobuf], dl
	mov rax, 1
	mov rdi, 1
	mov rdx, 1
	push rsi
	mov rsi, iobuf
	syscall
	pop rsi
	dec rsi
	jmp print_digit
print_end:
	mov rax, 1
	mov rdi, 1
	mov rdx, 1
	mov rsi, end_line
	syscall
	pop rsi
	pop rdx
	ret

getNum:
	;read a number for std input
	;store the number in rax
	;requires the input ended with \n
	;don't do any error check
	mov r12, 0 ;clear r12 to zero
read_loop:
	call getchar
	cmp rax, 10 ;10 means a new line
	je read_end
	cmp rax, 32 ;32 means a seperator
	je read_end
	imul r12, 10
	sub rax, 48 ;get the real number of input
	add r12, rax ; 10x + y
	jmp read_loop	
read_end:
	mov rax, r12
	ret 

getchar:
	;read a char for std input
	;store the char in rax
	mov rax, 0 ;read
	mov rdi, 1
	mov rsi, iobuf
	mov rdx, 1 ;one byte a time
	syscall
	movzx rax, byte[iobuf]
	ret  