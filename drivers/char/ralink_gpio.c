/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright, Ralink Technology, Inc.
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
 *
 */
#include <linux/init.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#ifdef CONFIG_RALINK_GPIO_LED
#include <linux/timer.h>
#endif
#include <asm/uaccess.h>
#include "ralink_gpio.h"

#include <asm/rt2880/surfboardint.h>

#ifdef  CONFIG_DEVFS_FS
#include <linux/devfs_fs_kernel.h>
static  devfs_handle_t devfs_handle;
#endif

#define NAME			"ralink_gpio"
#define RALINK_GPIO_DEVNAME	"gpio"
int ralink_gpio_major = 252;
u32 ralink_gpio_intp = 0;
int ralink_gpio_irqnum = 0;
u32 ralink_gpio_edge = 0;
ralink_gpio_reg_info ralink_gpio_info[RALINK_GPIO_NUMBER];
extern unsigned long volatile jiffies;
#ifdef CONFIG_RALINK_GPIO_LED
#define RALINK_LED_DEBUG 0
#define RALINK_GPIO_LED_FREQ (HZ/10)
struct timer_list ralink_gpio_led_timer;
#ifdef CONFIG_SW5_GPIO3_TIMER				
struct timer_list ralink_gpio_long_press_timer;
#endif
ralink_gpio_led_info ralink_gpio_led_data[RALINK_GPIO_NUMBER];
u32 ralink_gpio_led_value = 0;
struct ralink_gpio_led_status_t {
	int ticks;
	unsigned int ons;
	unsigned int offs;
	unsigned int resting;
	unsigned int times;
} ralink_gpio_led_stat[RALINK_GPIO_NUMBER];
#endif

MODULE_DESCRIPTION("Ralink SoC GPIO Driver");
MODULE_AUTHOR("Winfred Lu <winfred_lu@ralinktech.com.tw>");
MODULE_LICENSE("GPL");
ralink_gpio_reg_info info;

#ifdef CONFIG_SW5_GPIO3_TIMER				
int seconds;
int ralink_gpio_long_press_timer_act = 0;
void ralink_gpio_notify_user(int usr);
#endif
//#define RALINK_GPIO_DEBUG
#undef RALINK_GPIO_DEBUG

int ralink_gpio_ioctl(struct inode *inode, struct file *file, unsigned int req,
		unsigned long arg)
{
	unsigned long idx, tmp;
	ralink_gpio_reg_info info;
#ifdef CONFIG_RALINK_GPIO_LED
	ralink_gpio_led_info led;
#endif

	idx = (req >> RALINK_GPIO_DATA_LEN) & 0xFFL;
	req &= RALINK_GPIO_DATA_MASK;

	switch(req) {
	case RALINK_GPIO_SET_DIR:
		*(volatile u32 *)(RALINK_REG_PIODIR) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO_SET_DIR_IN:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODIR));
		tmp &= ~cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIODIR) = tmp;
		break;
	case RALINK_GPIO_SET_DIR_OUT:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODIR));
		tmp |= cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIODIR) = tmp;
		break;
	case RALINK_GPIO_READ: //RALINK_GPIO_READ_INT
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODATA));
		put_user(tmp, (int __user *)arg);
		break;
	case RALINK_GPIO_WRITE: //RALINK_GPIO_WRITE_INT
		*(volatile u32 *)(RALINK_REG_PIODATA) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO_SET: //RALINK_GPIO_SET_INT
		*(volatile u32 *)(RALINK_REG_PIOSET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO_CLEAR: //RALINK_GPIO_CLEAR_INT
		*(volatile u32 *)(RALINK_REG_PIORESET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO_READ_BIT:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODATA));
		if (0L <= idx && idx < RALINK_GPIO_DATA_LEN) {
			tmp = (tmp >> idx) & 1L;
			put_user(tmp, (int __user *)arg);
		}
		else
			return -EINVAL;
		break;
	case RALINK_GPIO_WRITE_BIT:
		if (0L <= idx && idx < RALINK_GPIO_DATA_LEN) {
			tmp =le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODATA));
			if (arg & 1L)
				tmp |= (1L << idx);
			else
				tmp &= ~(1L << idx);
			*(volatile u32 *)(RALINK_REG_PIODATA)= cpu_to_le32(tmp);
		}
		else
			return -EINVAL;
		break;
	case RALINK_GPIO_READ_BYTE:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODATA));
		if (0L <= idx && idx < RALINK_GPIO_DATA_LEN/8) {
			tmp = (tmp >> idx*8) & 0xFFL;
			put_user(tmp, (int __user *)arg);
		}
		else
			return -EINVAL;
		break;
	case RALINK_GPIO_WRITE_BYTE:
		if (0L <= idx && idx < RALINK_GPIO_DATA_LEN/8) {
			tmp =le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODATA));
			tmp &= ~(0xFFL << idx*8);
			tmp |= ((arg & 0xFFL) << idx*8);
			*(volatile u32 *)(RALINK_REG_PIODATA)= cpu_to_le32(tmp);
		}
		else
			return -EINVAL;
		break;
	case RALINK_GPIO_ENABLE_INTP:
		*(volatile u32 *)(RALINK_REG_INTENA) = cpu_to_le32(RALINK_INTCTL_PIO);
		break;
	case RALINK_GPIO_DISABLE_INTP:
		*(volatile u32 *)(RALINK_REG_INTDIS) = cpu_to_le32(RALINK_INTCTL_PIO);
		break;
	case RALINK_GPIO_REG_IRQ:
		copy_from_user(&info, (ralink_gpio_reg_info *)arg, sizeof(info));
		if (0 <= info.irq && info.irq < RALINK_GPIO_NUMBER) {
			ralink_gpio_info[info.irq].pid = info.pid;
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIORENA));
			tmp |= (0x1 << info.irq);
			*(volatile u32 *)(RALINK_REG_PIORENA) = cpu_to_le32(tmp);
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIOFENA));
			tmp |= (0x1 << info.irq);
			*(volatile u32 *)(RALINK_REG_PIOFENA) = cpu_to_le32(tmp);
		}
		else
			printk(KERN_ERR NAME ": irq number(%d) out of range\n",
					info.irq);
		break;
	case RALINK_GPIO_LED_SET:
#ifdef CONFIG_RALINK_GPIO_LED
		copy_from_user(&led, (ralink_gpio_led_info *)arg, sizeof(led));
		if (0 <= led.gpio && led.gpio < RALINK_GPIO_NUMBER) {
			if (led.on > RALINK_GPIO_LED_INFINITY)
				led.on = RALINK_GPIO_LED_INFINITY;
			if (led.off > RALINK_GPIO_LED_INFINITY)
				led.off = RALINK_GPIO_LED_INFINITY;
			if (led.blinks > RALINK_GPIO_LED_INFINITY)
				led.blinks = RALINK_GPIO_LED_INFINITY;
			if (led.rests > RALINK_GPIO_LED_INFINITY)
				led.rests = RALINK_GPIO_LED_INFINITY;
			if (led.times > RALINK_GPIO_LED_INFINITY)
				led.times = RALINK_GPIO_LED_INFINITY;
			if (led.on == 0 && led.off == 0 && led.blinks == 0 &&
					led.rests == 0) {
				ralink_gpio_led_data[led.gpio].gpio = -1; //stop it
				break;
			}
			//register led data
			ralink_gpio_led_data[led.gpio].gpio = led.gpio;
			ralink_gpio_led_data[led.gpio].on = led.on;
			ralink_gpio_led_data[led.gpio].off = led.off;
			ralink_gpio_led_data[led.gpio].blinks = led.blinks;
			ralink_gpio_led_data[led.gpio].rests = led.rests;
			ralink_gpio_led_data[led.gpio].times = led.times;

			//clear previous led status
			ralink_gpio_led_stat[led.gpio].ticks = -1;
			ralink_gpio_led_stat[led.gpio].ons = 0;
			ralink_gpio_led_stat[led.gpio].offs = 0;
			ralink_gpio_led_stat[led.gpio].resting = 0;
			ralink_gpio_led_stat[led.gpio].times = 0;

			//set gpio direction to 'out'
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODIR));
			tmp |= RALINK_GPIO(led.gpio);
			*(volatile u32 *)(RALINK_REG_PIODIR) = tmp;
#if RALINK_LED_DEBUG
			printk("dir_%x gpio_%d - %d %d %d %d %d\n", tmp,
					led.gpio, led.on, led.off, led.blinks,
					led.rests, led.times);
#endif
		}
		else
			printk(KERN_ERR NAME ": gpio number(%d) out of range\n",
					led.gpio);
#else
		printk(KERN_ERR NAME ": gpio led support not built\n");
#endif
		break;
	case RALINK_GPIO_IRQ_NUM:
		put_user(ralink_gpio_irqnum, (int __user *)arg);	
		break;
	default:
		return -ENOIOCTLCMD;
	}
	return 0;
}

int ralink_gpio_open(struct inode *inode, struct file *file)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	MOD_INC_USE_COUNT;
#else
	try_module_get(THIS_MODULE);
#endif
	return 0;
}

int ralink_gpio_release(struct inode *inode, struct file *file)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	MOD_DEC_USE_COUNT;
#else
	module_put(THIS_MODULE);
#endif
	return 0;
}

struct file_operations ralink_gpio_fops =
{
	owner:		THIS_MODULE,
	ioctl:		ralink_gpio_ioctl,
	open:		ralink_gpio_open,
	release:	ralink_gpio_release,
};

#ifdef CONFIG_RALINK_GPIO_LED

#if RALINK_GPIO_LED_LOW_ACT
#define __LED_ON(gpio) ralink_gpio_led_value &= ~RALINK_GPIO(gpio);
#define __LED_OFF(gpio) ralink_gpio_led_value |= RALINK_GPIO(gpio);
#else
#define __LED_ON(gpio) ralink_gpio_led_value |= RALINK_GPIO(gpio);
#define __LED_OFF(gpio) ralink_gpio_led_value &= ~RALINK_GPIO(gpio);
#endif

#ifdef CONFIG_SW5_GPIO3_TIMER				
static void ralink_gpio_long_press_do_timer(unsigned long unused)
{
	//printk("ralink_gpio_long_press_do_timer\n");
	if(ralink_gpio_long_press_timer_act == 1){
		
		if(seconds == CONFIG_GPIO_LONG_SEC) //time out
		{
			seconds=0;
			//kill some one	
			if(ralink_gpio_long_press_timer_act == 1){
				ralink_gpio_notify_user(2);
				ralink_gpio_long_press_timer_act = 0;
			}
		}else{
			
			seconds++;
		}
	}else{
		seconds = 0;
	}
	
	init_timer(&ralink_gpio_long_press_timer);
	ralink_gpio_long_press_timer.expires = jiffies + 200;
	add_timer(&ralink_gpio_long_press_timer);
		
}
#endif

#ifdef SUPPORT_CAMEO_SDK
#ifdef SUPPORT_DLINK_LED_SPEC	
#ifdef CONFIG_RALINK_GPIO_LED
void init_power_led(){
	unsigned long tmp;
	ralink_gpio_led_info led;
	int index;
	int i;

#ifdef SUPPORT_DUAL_COLOR_POWER_LED	
	for (i = 0; i < 2; i++){	
		memset(&led, 0, sizeof(led));
	
		if (i == 0){
			led.on = RALINK_GPIO_LED_INFINITY;			
			led.off = 0;
			index = CONFIG_POWER_LED_GPIO1;
		}else{
			led.on = 0;			
			led.off = RALINK_GPIO_LED_INFINITY;
			index = CONFIG_POWER_LED_GPIO2;
		}
		
		led.blinks = 0;			
		led.rests = 0;		
		led.times = RALINK_GPIO_LED_INFINITY;
					
		ralink_gpio_led_data[index].gpio = led.gpio;
		ralink_gpio_led_data[index].on = led.on;
		ralink_gpio_led_data[index].off = led.off;
		ralink_gpio_led_data[index].blinks = led.blinks;
		ralink_gpio_led_data[index].rests = led.rests;
		ralink_gpio_led_data[index].times = led.times;

		//clear previous led status
		ralink_gpio_led_stat[index].ticks = -1;
		ralink_gpio_led_stat[index].ons = 0;
		ralink_gpio_led_stat[index].offs = 0;
		ralink_gpio_led_stat[index].resting = 0;
		ralink_gpio_led_stat[index].times = 0;

		//set gpio direction to 'out'
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODIR));
		tmp |= RALINK_GPIO(index);
		*(volatile u32 *)(RALINK_REG_PIODIR) = tmp;
	}
#endif	// SUPPORT_DUAL_COLOR_POWER_LED

}
#endif	// CONFIG_RALINK_GPIO_LED
#endif	// SUPPORT_DLINK_LED_SPEC
#endif	// SUPPORT_CAMEO_SDK

static void ralink_gpio_led_do_timer(unsigned long unused)
{
	int i;
	unsigned int x;

	for (i = 0; i < RALINK_GPIO_NUMBER; i++) {
		ralink_gpio_led_stat[i].ticks++;
		if (ralink_gpio_led_data[i].gpio == -1) //-1 means unused
			continue;
		if (ralink_gpio_led_data[i].on == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].off == 0) { //always on
			__LED_ON(i);
			continue;
		}
		if (ralink_gpio_led_data[i].off == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].on == 0 ||
				ralink_gpio_led_data[i].blinks == 0 ||
				ralink_gpio_led_data[i].times == 0) { //always off
			__LED_OFF(i);
			continue;
		}

		//led turn on or off
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			x = ralink_gpio_led_stat[i].ticks % (ralink_gpio_led_data[i].on
					+ ralink_gpio_led_data[i].off);
		}
		else {
			unsigned int a, b, c, d, o, t;
			a = ralink_gpio_led_data[i].blinks / 2;
			b = ralink_gpio_led_data[i].rests / 2;
			c = ralink_gpio_led_data[i].blinks % 2;
			d = ralink_gpio_led_data[i].rests % 2;
			o = ralink_gpio_led_data[i].on + ralink_gpio_led_data[i].off;
			//t = blinking ticks
			t = a * o + ralink_gpio_led_data[i].on * c;
			//x = ticks % (blinking ticks + resting ticks)
			x = ralink_gpio_led_stat[i].ticks %
				(t + b * o + ralink_gpio_led_data[i].on * d);
			//starts from 0 at resting cycles
			if (x >= t)
				x -= t;
			x %= o;
		}
		if (x < ralink_gpio_led_data[i].on) {
			__LED_ON(i);
			if (ralink_gpio_led_stat[i].ticks && x == 0)
				ralink_gpio_led_stat[i].offs++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d on,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}
		else {
			__LED_OFF(i);
			if (x == ralink_gpio_led_data[i].on)
				ralink_gpio_led_stat[i].ons++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d off,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}

		//blinking or resting
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			continue;
		}
		else {
			x = ralink_gpio_led_stat[i].ons + ralink_gpio_led_stat[i].offs;
			if (!ralink_gpio_led_stat[i].resting) {
				if (x == ralink_gpio_led_data[i].blinks) {
					ralink_gpio_led_stat[i].resting = 1;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
					ralink_gpio_led_stat[i].times++;
				}
			}
			else {
				if (x == ralink_gpio_led_data[i].rests) {
					ralink_gpio_led_stat[i].resting = 0;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
				}
			}
		}
		if (ralink_gpio_led_stat[i].resting) {
			__LED_OFF(i);
#if RALINK_LED_DEBUG
			printk("resting,");
		} else {
			printk("blinking,");
#endif
		}

		//number of times
		if (ralink_gpio_led_data[i].times != RALINK_GPIO_LED_INFINITY)
		{
			if (ralink_gpio_led_stat[i].times ==
					ralink_gpio_led_data[i].times) {
				__LED_OFF(i);
				ralink_gpio_led_data[i].gpio = -1; //stop
			}
#if RALINK_LED_DEBUG
			printk("T%d\n", ralink_gpio_led_stat[i].times);
		} else {
			printk("T@\n");
#endif
		}
	}
	//always turn the power LED on
#ifdef CONFIG_RALINK_RT2880
	__LED_ON(12);
#elif defined (CONFIG_RALINK_RT3052) || defined (CONFIG_RALINK_RT2883)

#ifdef SUPPORT_CAMEO_SDK

#ifndef SUPPORT_CONTROL_GPIO9
	__LED_ON(9);
#endif	// SUPPORT_CONTROL_GPIO9

#else		// SUPPORT_CAMEO_SDK
	__LED_ON(9);
#endif	// SUPPORT_CAMEO_SDK
	
#endif

#ifdef SUPPORT_CAMEO_SDK
#ifdef SUPPORT_GPIO_3G_POWER	//always turn on the 3G module
	ralink_gpio_led_value &= ~RALINK_GPIO(7);
#endif	// SUPPORT_GPIO_3G_POWER
#endif // SUPPORT_CAMEO_SDK

	*(volatile u32 *)(RALINK_REG_PIODATA) = ralink_gpio_led_value;
#if RALINK_LED_DEBUG
	printk("led_value= %x\n", ralink_gpio_led_value);
#endif

	init_timer(&ralink_gpio_led_timer);
	ralink_gpio_led_timer.expires = jiffies + RALINK_GPIO_LED_FREQ;
	add_timer(&ralink_gpio_led_timer);
}

void ralink_gpio_led_init_timer(void)
{
	int i;

	for (i = 0; i < RALINK_GPIO_NUMBER; i++) {
		
#ifdef SUPPORT_TURN_POWER_LED_ON		
		if (i == CONFIG_POWER_LED_GPIO1 || i == CONFIG_POWER_LED_GPIO2){
			ralink_gpio_led_data[i].gpio = i;	// don't init these GPIOs to keep turn on the power led
			continue;
		}
#endif	// SUPPORT_TURN_POWER_LED_ON		
		
		ralink_gpio_led_data[i].gpio = -1; //-1 means unused
#if RALINK_GPIO_LED_LOW_ACT
		ralink_gpio_led_value |= RALINK_GPIO(i);
#else
		ralink_gpio_led_value &= ~RALINK_GPIO(i);
#endif
	}

#ifdef SUPPORT_CAMEO_SDK
#ifdef SUPPORT_GPIO_3G_POWER	//always turn on the 3G module
	ralink_gpio_led_value &= ~RALINK_GPIO(7);
#endif	// SUPPORT_GPIO_3G_POWER
#endif	// SUPPORT_CAMEO_SDK
	init_timer(&ralink_gpio_led_timer);
	ralink_gpio_led_timer.function = ralink_gpio_led_do_timer;
	ralink_gpio_led_timer.expires = jiffies + RALINK_GPIO_LED_FREQ;
	add_timer(&ralink_gpio_led_timer);
}
#endif


#ifdef CONFIG_SW5_GPIO3_TIMER				
void ralink_gpio_long_press_init_timer(void)
{
	init_timer(&ralink_gpio_long_press_timer);
	ralink_gpio_long_press_timer.function = ralink_gpio_long_press_do_timer;
	ralink_gpio_long_press_timer.expires = jiffies + 200;	//every one second check
	add_timer(&ralink_gpio_long_press_timer);
	
}
#endif

int __init ralink_gpio_init(void)
{
	unsigned int i;
	u32 gpiomode;

#ifdef  CONFIG_DEVFS_FS
	if (devfs_register_chrdev(ralink_gpio_major, RALINK_GPIO_DEVNAME,
				&ralink_gpio_fops)) {
		printk(KERN_ERR NAME ": unable to register character device\n");
		return -EIO;
	}
	devfs_handle = devfs_register(NULL, RALINK_GPIO_DEVNAME,
			DEVFS_FL_DEFAULT, ralink_gpio_major, 0,
			S_IFCHR | S_IRUGO | S_IWUGO, &ralink_gpio_fops, NULL);
#else
	int r = 0;
	r = register_chrdev(ralink_gpio_major, RALINK_GPIO_DEVNAME,
			&ralink_gpio_fops);
	if (r < 0) {
		printk(KERN_ERR NAME ": unable to register character device\n");
		return r;
	}
	if (ralink_gpio_major == 0) {
		ralink_gpio_major = r;
		printk(KERN_DEBUG NAME ": got dynamic major %d\n", r);
	}
#endif

	//config these pins to gpio mode
	gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_REG_GPIOMODE));
#if defined (CONFIG_RALINK_RT3052) || defined (CONFIG_RALINK_RT2883)
	gpiomode &= ~0x1C;  //clear bit[2:4]UARTF_SHARE_MODE
#endif
	gpiomode |= RALINK_GPIOMODE_DFT;
#ifdef CONFIG_CAMEO_DAUGHTERBOARD
	gpiomode &= ~(RALINK_GPIOMODE_I2C | RALINK_GPIOMODE_SPI);
#endif

#ifdef CONFIG_RALINK_GPIO_I2C
	gpiomode &= ~(RALINK_GPIOMODE_I2C);
#endif

#ifdef SUPPORT_CAMEO_SDK
#ifdef SUPPORT_GPIO_3G_POWER
	gpiomode |= RALINK_GPIO_7;	//gpio_7
#endif	// SUPPORT_GPIO_3G_POWER
#endif	// SUPPORT_CAMEO_SDK

	*(volatile u32 *)(RALINK_REG_GPIOMODE) = cpu_to_le32(gpiomode);

	//enable gpio interrupt
	*(volatile u32 *)(RALINK_REG_INTENA) = cpu_to_le32(RALINK_INTCTL_PIO);
	for (i = 0; i < RALINK_GPIO_NUMBER; i++) {
		ralink_gpio_info[i].irq = i;
		ralink_gpio_info[i].pid = 0;
	}

#ifdef SUPPORT_CAMEO_SDK
#ifdef SUPPORT_DLINK_LED_SPEC	
#ifdef CONFIG_RALINK_GPIO_LED
	init_power_led();
#endif	// CONFIG_RALINK_GPIO_LED	
#endif	// SUPPORT_DLINK_LED_SPEC
#endif	// SUPPORT_CAMEO_SDK

#ifdef CONFIG_RALINK_GPIO_LED
	ralink_gpio_led_init_timer();
#endif
#ifdef CONFIG_SW5_GPIO3_TIMER				
	
	ralink_gpio_long_press_init_timer();
#endif	
	printk("Ralink gpio driver initialized\n");
	return 0;
}

void __exit ralink_gpio_exit(void)
{
#ifdef  CONFIG_DEVFS_FS
	devfs_unregister_chrdev(ralink_gpio_major, RALINK_GPIO_DEVNAME);
	devfs_unregister(devfs_handle);
#else
	unregister_chrdev(ralink_gpio_major, RALINK_GPIO_DEVNAME);
#endif

	//config these pins to normal mode
	*(volatile u32 *)(RALINK_REG_GPIOMODE) &= ~RALINK_GPIOMODE_DFT;
	//disable gpio interrupt
	*(volatile u32 *)(RALINK_REG_INTDIS) = cpu_to_le32(RALINK_INTCTL_PIO);
#ifdef CONFIG_RALINK_GPIO_LED
	del_timer(&ralink_gpio_led_timer);
#endif
#ifdef CONFIG_SW5_GPIO3_TIMER				
	del_timer(&ralink_gpio_long_press_timer);
#endif
	printk("Ralink gpio driver exited\n");
}

/*
 * send a signal(SIGUSR1) to the registered user process whenever any gpio
 * interrupt comes
 * (called by interrupt handler)
 */
void ralink_gpio_notify_user(int usr)
{
	struct task_struct *p = NULL;

	if (ralink_gpio_irqnum < 0 || RALINK_GPIO_NUMBER <= ralink_gpio_irqnum) {
		printk(KERN_ERR NAME ": gpio irq number out of range\n");
		return;
	}

	//don't send any signal if pid is 0 or 1
	if ((int)ralink_gpio_info[ralink_gpio_irqnum].pid < 2)
		return;
	p = find_task_by_pid(ralink_gpio_info[ralink_gpio_irqnum].pid);
	if (NULL == p) {
		printk(KERN_ERR NAME ": no registered process to notify\n");
		return;
	}

	if (usr == 1) {
		#ifdef RALINK_GPIO_DEBUG
		printk(KERN_NOTICE NAME ": sending a SIGUSR1 to process %d\n",
				ralink_gpio_info[ralink_gpio_irqnum].pid);
		#endif
		send_sig(SIGUSR1, p, 0);
	}
	else if (usr == 2) {
		#ifdef RALINK_GPIO_DEBUG
		printk(KERN_NOTICE NAME ": sending a SIGUSR2 to process %d\n",
				ralink_gpio_info[ralink_gpio_irqnum].pid);
		#endif				
		send_sig(SIGUSR2, p, 0);
	}
}

/*
 * 1. save the PIOINT and PIOEDGE value
 * 2. clear PIOINT by writing 1
 * (called by interrupt handler)
 */
void ralink_gpio_save_clear_intp(void)
{
	ralink_gpio_intp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIOINT));
	ralink_gpio_edge = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIOEDGE));
	*(volatile u32 *)(RALINK_REG_PIOINT) = cpu_to_le32(0x00FFFFFF);
	*(volatile u32 *)(RALINK_REG_PIOEDGE) = cpu_to_le32(0x00FFFFFF);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
void ralink_gpio_irq_handler(unsigned int irq, struct irqaction *irqaction)
#else
irqreturn_t ralink_gpio_irq_handler(int irq, void *irqaction)
#endif
{
	struct gpio_time_record {
		unsigned long falling;
		unsigned long rising;
	};
	static struct gpio_time_record record[RALINK_GPIO_NUMBER];
	unsigned long now;
	int i;

	ralink_gpio_save_clear_intp();
	now = jiffies;
	for (i = 0; i < RALINK_GPIO_NUMBER; i++) {
		if (! (ralink_gpio_intp & (1 << i)))
			continue;
		ralink_gpio_irqnum = i;		
		if (ralink_gpio_edge & (1 << i)) { //rising edge			
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 30L)) {
				/*
				 * If the interrupt comes in a short period,
				 * it might be floating. We ignore it.
				 */
			}
			else {								
				record[i].rising = now;				
#ifdef SUPPORT_CAMEO_SDK
#ifdef CONFIG_GPIO_LONG_SEC
				
				if (time_before(now, record[i].falling + CONFIG_GPIO_LONG_SEC * 200L)) {
#else
				if (time_before(now, record[i].falling + 1000L)) {	// 200L = one second
#endif					
					//five click
#else
				if (time_before(now, record[i].falling + 200L)) {
					//one click
#endif  // SUPPORT_CAMEO_SDK

					ralink_gpio_notify_user(1);
				}
#ifdef CONFIG_SW5_GPIO3_TIMER				
				else{
					if(i != 3)
						ralink_gpio_notify_user(2);							
				}
				if((ralink_gpio_long_press_timer_act == 1) && (i==3)){					
					ralink_gpio_long_press_timer_act = 0;
				}
#else				
				else {
					//press for several seconds
					ralink_gpio_notify_user(2);
				}
#endif				
			}
		}
		else { //falling edge			
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 30L)) {
			}else{			
			record[i].falling = now;
#ifdef SUPPORT_CAMEO_SDK		
#ifdef CONFIG_CAMEO_DAUGHTERBOARD	
			ralink_gpio_notify_user(1);
#else		// CAMEO_DAUGHTERBOARD			
			if (i == 11){	//  only GPIO 11 will be triggered when it's falling edge for packet ap switch									
				ralink_gpio_notify_user(1);			
			}
			
#ifdef SUPPORT_CONTROL_RESET_LED			
			if (i == CONFIG_RESET_BTN_GPIO){	// when the reset button has been pressed
				ralink_gpio_notify_user(1);			
			}
#endif	// SUPPORT_CONTROL_RESET_LED			
#endif	// CAMEO_DAUGHTERBOARD	
#endif	// SUPPORT_CAMEO_SDK
#ifdef CONFIG_SW5_GPIO3_TIMER				
			/* press donw : start */	
			if((ralink_gpio_long_press_timer_act == 0) && (i == 3)){				
				ralink_gpio_long_press_timer_act = 1;				
//				printk("%d",i);

			}
#endif	
			}
		}
		break;
	}

	return IRQ_HANDLED;
}

void ralink_gpio_setmod(int actflag, u32 mode_value)
{
	u32 gpiomode;
	//config these pins to gpio mode
	gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_REG_GPIOMODE));

	if(actflag) //set to gpio
		gpiomode |= mode_value;
	else			
		gpiomode &= ~mode_value;

	*(volatile u32 *)(RALINK_REG_GPIOMODE) = cpu_to_le32(gpiomode);

}

struct irqaction ralink_gpio_irqaction = {
	.handler = ralink_gpio_irq_handler,
	.flags = SA_INTERRUPT,
	.mask = 0,
	.name = "ralink_gpio",
};

void __init ralink_gpio_init_irq(void)
{
	setup_irq(SURFBOARDINT_GPIO, &ralink_gpio_irqaction);
}

EXPORT_SYMBOL(ralink_gpio_setmod);
module_init(ralink_gpio_init);
module_exit(ralink_gpio_exit);

