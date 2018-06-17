
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "string.h"
#include "proc.h"
#include "global.h"

#define CHAIR 2
#define TIME_SLICE 40000

int count;
int c_id;
Semaphore customer, barber, mutex;

PRIVATE void init()
{
    count = 0;
    c_id = 0;
    customer.x = barber.x = 0;
    mutex.x = 1;

    memset(0xB8000, 0, 80 * 25 * 2);
    disp_pos = 0;
}

/*======================================================================*
                            kernel_main
 *======================================================================*/
PUBLIC int kernel_main()
{
	disp_str("-----\"kernel_main\" begins-----\n");

	TASK*		p_task		= task_table;
	PROCESS*	p_proc		= proc_table;
	char*		p_task_stack	= task_stack + STACK_SIZE_TOTAL;
	u16		selector_ldt	= SELECTOR_LDT_FIRST;
	int i;
	for (i = 0; i < NR_TASKS; i++) {
		strcpy(p_proc->p_name, p_task->name);	// name of the process
		p_proc->pid = i;			// pid

		p_proc->ldt_sel = selector_ldt;

        p_proc->blocked = 0; // don't blocked in the first place

        p_proc->ticks = 0; // ticks = 0 in the beginning

		memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[0].attr1 = DA_C | PRIVILEGE_TASK << 5;
		memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[1].attr1 = DA_DRW | PRIVILEGE_TASK << 5;
		p_proc->regs.cs	= ((8 * 0) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.ds	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.es	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.fs	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.ss	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK)
			| RPL_TASK;

		p_proc->regs.eip = (u32)p_task->initial_eip;
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = 0x1202; /* IF=1, IOPL=1 */

		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}


    init();

	k_reenter = 0;
	ticks = 0;

	p_proc_ready	= proc_table;

        /* 初始化 8253 PIT */
        out_byte(TIMER_MODE, RATE_GENERATOR);
        out_byte(TIMER0, (u8) (TIMER_FREQ/HZ) );
        out_byte(TIMER0, (u8) ((TIMER_FREQ/HZ) >> 8));

        put_irq_handler(CLOCK_IRQ, clock_handler); /* 设定时钟中断处理程序 */
        enable_irq(CLOCK_IRQ);                     /* 让8259A可以接收时钟中断 */

	restart();

        while(1){}
}

void A()
{
    while (1) {
        user_disp_str("Are you OK?\n");
        milli_delay(TIME_SLICE);
    }
}

void Barber()
{
    user_disp_str("Barber sleeping...\n");
    process_sleep(TIME_SLICE);
    user_disp_str("Barber awake...\n");
    while(1){
        sem_p(&customer);
        user_disp_str("Barber begins to cut hair...\n");
        process_sleep(2 * TIME_SLICE);
        user_disp_str("Barber haircut done!\n");
        sem_v(&barber);
    }
}

void Customer()
{
    while (1) {
        sem_p(&mutex);
        c_id++;

        int id = c_id;
        user_disp_str("Customer ");
        user_disp_int(id);
        user_disp_str(" come.\nCurrent waiting count: ");
        user_disp_int(count);
        if (count >= CHAIR) {
            user_disp_str(", no chair, leave\n");
            sem_v(&mutex);
        } else {
            count++;
            user_disp_str(", waiting\n");
            sem_v(&mutex);

            sem_v(&customer);
            sem_p(&barber);

            user_disp_str("Customer ");
            user_disp_int(id);
            user_disp_str(" got haircut\n");

            sem_p(&mutex);
            count--;
            sem_v(&mutex);
        }
        process_sleep(TIME_SLICE);
    }
}

