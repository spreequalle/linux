/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2006, Ralink Technology, Inc.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 ***************************************************************************

    Module Name:
    rt_timer.c

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ----------------------------------------------
    Name        Date            Modification logs
    Steven Liu  2007-07-04      Initial version
*/

#include <linux/config.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/rt2880/surfboardint.h>
#include <linux/interrupt.h>
#include "rt_timer.h"


#if defined (CONFIG_RALINK_TIMER_WDG)   
    static int wdg_load_value;
    struct timer_list wdg_timer;
#endif
#if defined (CONFIG_RALINK_TIMER_DFS)   
    static struct timer0_data tmr0;
#endif

void set_wdg_timer_ebl(unsigned int timer, unsigned int ebl)
{
    unsigned int result;

    result=sysRegRead(timer);

    if(ebl==1){
	result |= (1<<7);
    }else {
	result &= ~(1<<7);
    }

    sysRegWrite(timer,result);

    //timer1 used for watchdog timer
#if defined (CONFIG_RALINK_TIMER_WDG_RESET_OUTPUT)

#if defined (CONFIG_RALINK_RT2880)
    if(timer==TMR1CTL) {
	result=sysRegRead(CLKCFG);            

	if(ebl==1){
	    result |= (1<<9); /* SRAM_CS_N is used as wdg reset */
	}else {
	    result &= ~(1<<9); /* SRAM_CS_N is used as normal func */
	}

	sysRegWrite(CLKCFG,result);
    }
#elif defined (CONFIG_RALINK_RT3052_MP2) || defined(CONFIG_RALINK_RT2883)
    if(timer==TMR1CTL) {
	//the last 4bits in SYSCFG are write only
	result=sysRegRead(SYSCFG);                                                                                    

	if(ebl==1){
	    result |= (1<<2); /* SRAM_CS_MODE is used as wdg reset */
	}

	sysRegWrite(SYSCFG,result);
    }
#endif             
#endif 
}


void set_wdg_timer_clock_prescale(unsigned int timer, enum timer_clock_freq prescale)
{
    unsigned int result;

    result=sysRegRead(timer);
    result &= ~0xF;
    result=result | (prescale&0xF);
    sysRegWrite(timer,result);

}

void set_wdg_timer_mode(unsigned int timer, enum timer_mode mode)
{
    unsigned int result;

    result=sysRegRead(timer);
    result &= ~(0x3<<4);
    result=result | (mode << 4);
    sysRegWrite(timer,result);

}

void setup_wdg_timer(struct timer_list * timer,
	void (*function)(unsigned long),
	unsigned long data)
{
    timer->function = function;
    timer->data = data;
    init_timer(timer);
}

#if defined (CONFIG_RALINK_TIMER_WDG)
void refresh_wdg_timer(unsigned long unused)
{
    sysRegWrite(TMR1LOAD, wdg_load_value);

    wdg_timer.expires = jiffies + HZ * CONFIG_RALINK_WDG_REFRESH_INTERVAL;
    add_timer(&wdg_timer);
}
#endif

#if defined (CONFIG_RALINK_TIMER_DFS)   
int request_tmr_service(int interval, void (*function)(unsigned long), unsigned long data)
{
    unsigned int reg_val;
    unsigned long flags;

    spin_lock_irqsave(tmr0.tmr0_lock, flags);

    //Set Callback function
    tmr0.data = data;
    tmr0.tmr0_callback_function = function;

    //Timer 0 Interrupt Status Enable
    reg_val = sysRegRead(INTENA);
    reg_val |= 1;
    sysRegWrite(INTENA, reg_val);

    //Set Timer0 Mode
    set_wdg_timer_mode(TMR0CTL, PERIODIC);

    //Set Period Interval
    //Unit= SysClk/16384, 1ms = (SysClk/16384)/1000
    set_wdg_timer_clock_prescale(TMR0CTL,SYS_CLK_DIV16384);

    sysRegWrite(TMR0LOAD, interval* (get_surfboard_sysclk()/16384/1000));

    //Enable Timer0
    set_wdg_timer_ebl(TMR0CTL,1);

    spin_unlock_irqrestore(tmr0.tmr0_lock, flags);

    return 0;
}

int unregister_tmr_service(void)
{
    unsigned int reg_val=0;
    unsigned long flags=0;

    spin_lock_irqsave(tmr0.tmr0_lock, flags);

    //Disable Timer0
    set_wdg_timer_ebl(TMR0CTL,0);

    //Timer0 Interrupt Status Disable
    reg_val = sysRegRead(INTENA);
    reg_val &= 0xfffe;
    sysRegWrite(INTENA, reg_val);

    //Unregister Callback Function
    tmr0.data = 0;
    tmr0.tmr0_callback_function = NULL;

    spin_unlock_irqrestore(tmr0.tmr0_lock, flags);

    return 0;
}

#if defined (CONFIG_RALINK_TIMER_DFS)   
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,21)
static irqreturn_t rt2880tmr0_irq_handler(int irq, void *dev_id)
#else
static irqreturn_t rt2880tmr0_irq_handler(int irq, void *dev_id, struct pt_regs * regs)
#endif
{
    unsigned long flags;
    unsigned int reg_val;

    spin_lock_irqsave(tmr0.tmr0_lock, flags);

    //Writing '1' to TMRSTAT[0]:TMR0INT to clear the interrupt
    reg_val = sysRegRead(TMRSTAT);
    reg_val |= 1; 
    sysRegWrite(TMRSTAT, reg_val);

    //execute callback function
    if ( tmr0.tmr0_callback_function != NULL) {
	(tmr0.tmr0_callback_function)(tmr0.data);
    }

    spin_unlock_irqrestore(tmr0.tmr0_lock, flags);

    return IRQ_HANDLED;

}
#endif
#endif

int32_t __init timer_init_module(void)
{
    printk("Load RT2880 Timer Module(Wdg/Soft)\n");

    /* 
     * System Clock = CPU Clock/2
     * For user easy configuration, We assume the unit of watch dog timer is 1s, 
     * so we need to calculate the TMR1LOAD value.
     *
     * Unit= 1/(SysClk/65536), 1 Sec = (SysClk)/65536 
     *
     */

#if defined (CONFIG_RALINK_TIMER_WDG)   
    // initialize WDG timer (Timer1)
    setup_wdg_timer(&wdg_timer, refresh_wdg_timer, 0);
    set_wdg_timer_mode(TMR1CTL,WATCHDOG);
    set_wdg_timer_clock_prescale(TMR1CTL,SYS_CLK_DIV65536);
    wdg_load_value = CONFIG_RALINK_WDG_TIMER * (get_surfboard_sysclk()/65536);
    refresh_wdg_timer(wdg_load_value);
    set_wdg_timer_ebl(TMR1CTL,1);
#endif

#if defined (CONFIG_RALINK_TIMER_DFS)   
    // initialize Soft Timer (Timer0)
    spin_lock_init(&tmr0.tmr0_lock);
    if(request_irq(SURFBOARDINT_TIMER0, rt2880tmr0_irq_handler, SA_INTERRUPT,
		"rt2880_timer0", NULL)){
	return 1;
    }
#endif

    return 0;
}

void __exit timer_cleanup_module(void)
{
    printk("Unload RT2880 Timer Module(Wdg/Soft)\n");

#if defined (CONFIG_RALINK_TIMER_WDG)   
    set_wdg_timer_ebl(TMR1CTL,0);
    del_timer_sync(&wdg_timer);
#endif

#if defined (CONFIG_RALINK_TIMER_DFS)   
    unregister_tmr_service();
#endif
}

module_init(timer_init_module);
module_exit(timer_cleanup_module);

#if defined (CONFIG_RALINK_TIMER_DFS)   
EXPORT_SYMBOL(request_tmr_service);
EXPORT_SYMBOL(unregister_tmr_service);
#endif

MODULE_DESCRIPTION("Ralink Timer Module(Wdg/Soft)");
MODULE_AUTHOR("Steven/Bob");
MODULE_LICENSE("GPL");
