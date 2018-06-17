
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               proc.c
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

PRIVATE int get_color() 
{
   int colors[] = {9, 10, 11, 12, 13};
   return colors[p_proc_ready->pid % 5];
}

/*======================================================================*
                              schedule
 *======================================================================*/
PUBLIC void schedule()
{
    for (PROCESS* p = proc_table; p < proc_table + NR_TASKS; p++) {
        if (p->ticks > 0) {
            if (p->ticks < ticks) {
	    		p->ticks = 0;
				p->blocked = 0;
            }
        }
    }

    p_proc_ready++;
    if (p_proc_ready >= proc_table + NR_TASKS) {
        p_proc_ready = proc_table;
    }
    while (p_proc_ready->ticks > 0 || p_proc_ready->blocked == 1) {
        p_proc_ready++;
        if (p_proc_ready >= proc_table + NR_TASKS) {
            p_proc_ready = proc_table;
        }
    }
}

/*======================================================================*
                           sys_get_ticks
 *======================================================================*/
PUBLIC int sys_get_ticks()
{
    return ticks;
}

PUBLIC void sys_process_sleep(int mill_seconds)
{
    p_proc_ready->ticks = mill_seconds * HZ / 1000 + ticks;
    schedule();
}

PUBLIC void sys_disp_str(char* str)
{
    if (disp_pos >= 80 * 25 * 2) {
        memset(0xB8000, 0, 80 * 25 * 2);
        disp_pos = 0;
    }
    
    disp_color_str(str, get_color());
    disp_str("");
}



PUBLIC void sys_disp_int(int value) 
{
    disp_int(value);
}

PUBLIC void sys_sem_p(Semaphore * s)
{
    s->x--;
    if (s->x < 0) {
        s->queue[- s->x - 1] = p_proc_ready;
        p_proc_ready->blocked = 1;
        schedule(1);
    }
}

PUBLIC void sys_sem_v(Semaphore * s)
{
    s->x++;
    if (s->x <= 0) {
        s->queue[0]->blocked = 0;
        for (int i = 0; i <-s->x; i++) {
            s->queue[i] = s->queue[i + 1];
        }
        schedule();
    }
}
