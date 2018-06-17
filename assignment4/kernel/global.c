
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            global.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#define GLOBAL_VARIABLES_HERE

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "proc.h"
#include "global.h"


PUBLIC	PROCESS			proc_table[NR_TASKS];

PUBLIC	char			task_stack[STACK_SIZE_TOTAL];

PUBLIC	TASK	task_table[NR_TASKS] = {{A, STACK_SIZE_TESTA, "A"},
                                        {Barber, STACK_SIZE_TESTB, "B"},
                                        {Customer, STACK_SIZE_TESTC, "C"},
                                        {Customer, STACK_SIZE_TESTD, "D"},
                                        {Customer, STACK_SIZE_TESTE, "E"}};

PUBLIC	irq_handler		irq_table[NR_IRQ];

PUBLIC	system_call		sys_call_table[NR_SYS_CALL] = {sys_get_ticks, sys_process_sleep, sys_disp_str, sys_disp_int, sys_sem_p, sys_sem_v};

