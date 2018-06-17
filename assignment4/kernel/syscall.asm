
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                               syscall.asm
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                                                     Forrest Yu, 2005
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

%include "sconst.inc"

_NR_get_ticks       equ 0 ; 要跟 global.c 中 sys_call_table 的定义相对应！
_NR_process_sleep 	equ 1
_NR_disp_str 		equ 2
_NR_sem_p			equ 3 ; P操作
_NR_sem_v			equ 4 ; V操作
INT_VECTOR_SYS_CALL equ 0x90

; 导出符号
global	get_ticks
global	process_sleep
global 	user_disp_str
global	sem_p
global	sem_v

bits 32
[section .text]

; ====================================================================
;                              get_ticks
; ====================================================================
get_ticks:
	mov	eax, _NR_get_ticks
	int	INT_VECTOR_SYS_CALL
	ret

process_sleep: ; void process_sleep(int milli_seconds);
	mov eav, _NR_process_sleep
	mov ebx, [esp + 4] ; 参数，睡眠的毫秒数
	int	INT_VECTOR_SYS_CALL
	ret

user_disp_str: ; void user_disp_str(char * str);
	mov eav, _NR_disp_str
	mov ebx, [esp + 4] ; 参数，字符串首地址
	int	INT_VECTOR_SYS_CALL
	ret

sem_p: ; void sem_p(int i);
	mov eav, _NR_sem_p
	mov ebx, [esp + 4] ; 参数，获取的信号量的编号
	int	INT_VECTOR_SYS_CALL

sem_v: ; void sem_v(int i);
	mov eav, _NR_sem_v
	mov ebx, [esp + 4] ; 参数，释放的信号量的编号
	int	INT_VECTOR_SYS_CALL
