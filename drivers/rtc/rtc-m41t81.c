/*
 * An RTC test device/driver
 * Copyright (C) 2005 Tower Technologies
 * Author: Alessandro Zummo <a.zummo@towertech.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/err.h>
#include <linux/rtc.h>
#include <linux/platform_device.h>

#include <asm/rt2880/rt_mmap.h>

/*---------------------------------------------------------------------*/
/* Symbol & Macro Definitions                                          */
/*---------------------------------------------------------------------*/

#define	RT2880_REG(x)						(*((volatile u32 *)(x)))
#define	RT2880_RSTCTRL_REG		(RALINK_SYSCTL_BASE+0x34)

#define RSTCTRL_I2C_RESET		RALINK_I2C_RST

#define RT2880_I2C_REG_BASE		(RALINK_I2C_BASE)
#define RT2880_I2C_CONFIG_REG		(RT2880_I2C_REG_BASE+0x00)
#define RT2880_I2C_CLKDIV_REG		(RT2880_I2C_REG_BASE+0x04)
#define RT2880_I2C_DEVADDR_REG		(RT2880_I2C_REG_BASE+0x08)
#define RT2880_I2C_ADDR_REG		(RT2880_I2C_REG_BASE+0x0C)
#define RT2880_I2C_DATAOUT_REG	 	(RT2880_I2C_REG_BASE+0x10)
#define RT2880_I2C_DATAIN_REG  		(RT2880_I2C_REG_BASE+0x14)
#define RT2880_I2C_STATUS_REG  		(RT2880_I2C_REG_BASE+0x18)
#define RT2880_I2C_STARTXFR_REG		(RT2880_I2C_REG_BASE+0x1C)
#define RT2880_I2C_BYTECNT_REG		(RT2880_I2C_REG_BASE+0x20)


/* I2C_CFG register bit field */
#define I2C_CFG_ADDRLEN_8				(7<<5)	/* 8 bits */
#define I2C_CFG_DEVADLEN_7				(6<<2)	/* 7 bits */
#define I2C_CFG_ADDRDIS					(1<<1)	/* disable address transmission*/
#define I2C_CFG_DEVADDIS				(1<<0)	/* disable evice address transmission */


#define IS_BUSY		(RT2880_REG(RT2880_I2C_STATUS_REG) & 0x01)
#define IS_SDOEMPTY	(RT2880_REG(RT2880_I2C_STATUS_REG) & 0x02)
#define IS_DATARDY	(RT2880_REG(RT2880_I2C_STATUS_REG) & 0x04)


/*
 * max SCLK : 400 KHz (2.7V)
 * assumed that BUS CLK is 150 MHZ 
 * so DIV 375
 * SCLK = PB_CLK / CLKDIV -> CLKDIV = PB_CLK / SCLK = PB_CLK / 0.4
 */

/*
 * Example :
 * 	In RT3052, System clock is 40 / 3 = 13.3
 *	Hence, CLKDIV = 13.3 / 0.4 = 33	
 * 	In RT2880, System Clock is 133Mhz
 *	Hence, CLKDIV = 133 / 0.4 = 332.5 -> Use 333 ( If use 150Mhz, then 150 / 0.4 = 375 )
 */

#define CLKDIV_VALUE	160 //375

#define i2c_busy_loop 	(CLKDIV_VALUE*30)
#define max_ee_busy_loop	(CLKDIV_VALUE*25)
						  

/* 
 * AT24C01A/02/04/08A/16A (1K, 2K, 4K, 8K, 16K) 
 *	-- address : 8-bits
 * AT24C512 (512K)
 *  -- address : two 8-bits
 */    
#define ADDRESS_BYTES	1

/* 
 * sequential reads
 * because BYTECNT REG max 64 (6-bits)
 * , max READ_BLOCK is 64 
 */
#define READ_BLOCK		16

/*
 * AT24C01A/02 (1K, 2K)  have 8-byte Page
 */
#define WRITE_BLOCK		8


/*
 * ATMEL AT25XXXX Serial EEPROM 
 * access type
 */

/* Instruction codes */
#define READ_CMD	0x01
#define WRITE_CMD	0x00


#define I2C_CFG_DEFAULT			(I2C_CFG_ADDRLEN_8  | \
								 I2C_CFG_DEVADLEN_7 | \
								 I2C_CFG_ADDRDIS)



#define RTC_ADDR		(0xD0>>1)
/* gloable variable*/
unsigned long i2cdrv_addr = RTC_ADDR;
/*----------------------------------------------------------------------*/
/*   Function                                                           */
/*           	i2c_master_init                                         */
/*    INPUTS: None                                                      */
/*   RETURNS: None                                                      */
/*   OUTPUTS: None                                                      */
/*   NOTE(S): Initialize I2C block to desired state                     */
/*                                                                      */
/*----------------------------------------------------------------------*/
void i2c_master_init(void)
{
	u32 i;
	/* reset i2c block */
	i = RT2880_REG(RT2880_RSTCTRL_REG) | RSTCTRL_I2C_RESET;
    RT2880_REG(RT2880_RSTCTRL_REG) = i;

    // force to clear i2c reset bit for RT2883.
    RT2880_REG(RT2880_RSTCTRL_REG) = i & ~(RSTCTRL_I2C_RESET);

	for(i = 0; i < 50000; i++);
	// udelay(500);
	
	RT2880_REG(RT2880_I2C_CONFIG_REG) = I2C_CFG_DEFAULT;

	RT2880_REG(RT2880_I2C_CLKDIV_REG) = CLKDIV_VALUE;

	/*
	 * Device Address : 
	 *
	 * ATMEL 24C152 serial EEPROM
	 *       1|0|1|0|0|A1|A2|R/W
	 *      MSB              LSB
	 * 
	 * ATMEL 24C01A/02/04/08A/16A
	 *    	MSB               LSB	  
	 * 1K/2K 1|0|1|0|A2|A1|A0|R/W
	 * 4K            A2 A1 P0
	 * 8K            A2 P1 P0
	 * 16K           P2 P1 P0 
	 *
	 * so device address needs 7 bits 
	 * if device address is 0, 
	 * write 0xA0 >> 1 into DEVADDR(max 7-bits) REG  
	 */
	RT2880_REG(RT2880_I2C_DEVADDR_REG) = i2cdrv_addr;

	/*
	 * Use Address Disabled Transfer Options
	 * because it just support 8-bits, 
	 * ATMEL eeprom needs two 8-bits address
	 */
	RT2880_REG(RT2880_I2C_ADDR_REG) = 0;
}

/*----------------------------------------------------------------------*/
/*   Function                                                           */
/*           	i2c_write                                               */
/*    INPUTS: 8-bit data                                                */
/*   RETURNS: None                                                      */
/*   OUTPUTS: None                                                      */
/*   NOTE(S): transfer 8-bit data to I2C                                */
/*                                                                      */
/*----------------------------------------------------------------------*/
static void i2c_write(u32 address, u8 *data, u32 nbytes)
{
	int i, j;
	u32 n;

	/* two bytes data at least so NODATA = 0 */
	n = nbytes + ADDRESS_BYTES;
	RT2880_REG(RT2880_I2C_BYTECNT_REG) = n-1;
	if (ADDRESS_BYTES == 2)
		RT2880_REG(RT2880_I2C_DATAOUT_REG) = (address >> 8) & 0xFF;
	else
		RT2880_REG(RT2880_I2C_DATAOUT_REG) = address & 0xFF;

	RT2880_REG(RT2880_I2C_STARTXFR_REG) = WRITE_CMD;
	for (i=0; i<n-1; i++) {
		j = 0;
		do {
			if (IS_SDOEMPTY) {
				if (ADDRESS_BYTES == 2) {
					if (i==0) {
						RT2880_REG(RT2880_I2C_DATAOUT_REG) = address & 0xFF;
					} else {
						RT2880_REG(RT2880_I2C_DATAOUT_REG) = data[i-1];
					}								
				} else {
					RT2880_REG(RT2880_I2C_DATAOUT_REG) = data[i];
				}
 			break;
			}
		} while (++j<max_ee_busy_loop);
	}

	i = 0;
	while(IS_BUSY && i<i2c_busy_loop){
		i++;
	};
}

/*----------------------------------------------------------------------*/
/*   Function                                                           */
/*           	i2c_read                                                */
/*    INPUTS: None                                                      */
/*   RETURNS: 8-bit data                                                */
/*   OUTPUTS: None                                                      */
/*   NOTE(S): get 8-bit data from I2C                                   */
/*                                                                      */
/*----------------------------------------------------------------------*/
static void i2c_read(u8 *data, u32 nbytes) 
{
	int i, j;

	RT2880_REG(RT2880_I2C_BYTECNT_REG) = nbytes-1;
	RT2880_REG(RT2880_I2C_STARTXFR_REG) = READ_CMD;
	for (i=0; i<nbytes; i++) {
		j = 0;
		do {
			if (IS_DATARDY) {
				data[i] = RT2880_REG(RT2880_I2C_DATAIN_REG);
				break;
			}
		} while(++j<max_ee_busy_loop);
	}

	i = 0;
	while(IS_BUSY && i<i2c_busy_loop){
		i++;
	};
}


static u8 i2c_read_one_byte(u32 address)
{	
	u8 data, iData[20];
	int i, j;
	u32 n;
	i2c_master_init();
	
  	   	
	i2c_write(address, &data, 0);
//	i2c_read(iData, 8);
	i2c_read(&data, 1);
	
//	for(i=0; i<8; i++){
//		printk("read[%d]: %x \n",i,iData[i]);
//	}
//	data = iData[0];
//	printk("\n\n\n");
	return (data);
}




//=======
static struct platform_device *test0 = NULL, *test1 = NULL;

static int test_rtc_read_alarm(struct device *dev,
	struct rtc_wkalrm *alrm)
{
	return 0;
}

static int test_rtc_set_alarm(struct device *dev,
	struct rtc_wkalrm *alrm)
{
	return 0;
}

int bcd2int(u8 bcd){
	
	u8 bcdhi, bcdlo;	
	int value;
	bcdhi = bcd >> 4;
	bcdlo = bcd & 0xF;
	if(bcdhi > 9) bcdhi = 9;
	if(bcdlo > 9) bcdhi = 9;
	value = bcdhi*10 + bcdlo;
	return value;
	
}

u8 int2bcd(int in){
	u8 bcdhi, bcdlo, bcd;	
	bcdhi = in/10;
	bcdlo = in%10;
	
	bcd = (bcdhi<<4) | bcdlo;
	
	return bcd;			
}

static u8 i2c_read_time(struct rtc_time *tm)
{
	u8 buf[8];
	u32 address = 0;
	i2c_write(address, buf, 0);
	i2c_read(buf, 8);
	
	
//	tm->tm_sec = bcd2int(buf[1]);
//	tm->tm_min = bcd2int(buf[2]);
//	tm->tm_hour = bcd2int(buf[3]);
//	tm->tm_mday = bcd2int(buf[5]);
//	tm->tm_mon = bcd2int(buf[6]);
//	tm->tm_year = bcd2int(buf[7]);
//	tm->tm_wday = bcd2int(buf[4]) + 2000;
	tm->tm_sec = bcd2int(buf[1] & 0x7F);
	tm->tm_min = bcd2int(buf[2] & 0x7F);
	tm->tm_hour = bcd2int(buf[3] & 0x3F);
	tm->tm_mday = bcd2int(buf[5] & 0x3F);
	tm->tm_mon = bcd2int(buf[6] & 0x1F) - 1;
	tm->tm_year = bcd2int(buf[7]) + 100;
	tm->tm_wday = bcd2int(buf[4] & 0x07);
	//tm->tm_yday = buf[1];
	//tm->tm_isdst = buf[1];
	

	
}
static int test_rtc_read_time(struct device *dev,
	struct rtc_time *tm)
{
	
	
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
	int buf;
	u8 data;
	
	u8 sec = 0;
	//printk("\n\n\n\n seconds : %x\n", sec);
	//i2c_read_one_byte(0x0);
	
	//data = 0x00;
	//i2c_write(0x0C, &data, 1);	//start up the rtc	
	
	
//	buf = i2c_read_one_byte(0x01); 
//	tm_sec = bcd2int(buf&0x7F);	
//	printk("tm_sec=%d\n",tm_sec);
//
//	buf = i2c_read_one_byte(0x02); 
//	tm_min = bcd2int(buf&0x7F);
//	printk("tm_min=%d\n",tm_min);
//
//	buf = i2c_read_one_byte(0x03); 
//	tm_hour = bcd2int(buf&0x3F);
//	printk("tm_hour=%d\n",tm_hour);
//	
//	buf = i2c_read_one_byte(0x05); 
//	tm_mday = bcd2int(buf&0x3F);
//	printk("tm_mday=%d\n",tm_mday);
//	
//	buf = i2c_read_one_byte(0x06); 
//	tm_mon = bcd2int(buf&0x1F);
//	printk("tm_mon=%d\n",tm_mon);
//	
//	buf = i2c_read_one_byte(0x07); 
//	tm_year = bcd2int(buf&0xFF);
//	printk("tm_year=%d\n",tm_year);
//
//	buf = i2c_read_one_byte(0x04); 
//	tm_wday = bcd2int(buf&0x03);
//	printk("tm_wday=%d\n",tm_wday);

//	tm->tm_sec = tm_sec;
//	tm->tm_min = tm_min;
//	tm->tm_hour = tm_hour;
//	tm->tm_mday = tm_mday;
//	tm->tm_mon = tm_mon;
//	tm->tm_year = tm_year;
//	tm->tm_wday = tm_wday;
	
	i2c_read_time(tm);
//	tm->tm_sec = 59;
//	tm->tm_min = 0;
//	tm->tm_hour = 0;
//	tm->tm_mday = 1;
//	tm->tm_mon = 1;
//	tm->tm_year = 2009;
//	tm->tm_wday = 0;

	
	//tm_yday = i2c_read_one_byte(0x0100); 
	//tm_year = bcd2int(buf&0xFF);
	//printk("tm_mon=%d\n",tm_mon);

	//rtc_time_to_tm(get_seconds(), tm);
	data = 0x00;
	i2c_write(0x0C, &data, 1);	//start up the rtc	
	return 0;
}

static int test_rtc_set_time(struct device *dev,
	struct rtc_time *tm)
{
	
	int data;
	u8 bcddata;
	
	data = tm->tm_sec;
	bcddata = int2bcd(data);
	i2c_write(0x01, &bcddata, 1);
	printk("tm_sec = %x\n",tm->tm_sec);
	
	data = tm->tm_min;
	bcddata = int2bcd(data);
	i2c_write(0x02, &bcddata, 1);
	printk("tm_min = %x\n",tm->tm_min);

	data = tm->tm_hour;
	bcddata = int2bcd(data);
	i2c_write(0x03, &bcddata, 1);
printk("tm_hour = %x\n",tm->tm_hour);

	data = tm->tm_mday;
	bcddata = int2bcd(data);
	i2c_write(0x05, &bcddata, 1);
printk("tm_mday = %x\n",tm->tm_mday);


	data = tm->tm_mon + 1;
	bcddata = int2bcd(data);
	i2c_write(0x06, &bcddata, 1);
printk("tm_mon = %x\n",tm->tm_mon);
	data = tm->tm_year - 100;
	bcddata = int2bcd(data);
	i2c_write(0x07, &bcddata, 1);
printk("tm_year = %x\n",tm->tm_year);
	data = tm->tm_wday ;
	bcddata = int2bcd(data);
	i2c_write(0x04, &bcddata, 1);
printk("tm_wday = %x\n",tm->tm_wday);	

	data = 0x00;
	i2c_write(0x0C, &data, 1);	//start up the rtc	
	
	printk("\n\n\n\n test_rtc_set_time \n");
	return 0;
}

static int test_rtc_set_mmss(struct device *dev, unsigned long secs)
{
	printk("\n\n\n\n test_rtc_set_time \n");
	return 0;
}

static int test_rtc_proc(struct device *dev, struct seq_file *seq)
{
	struct platform_device *plat_dev = to_platform_device(dev);

	seq_printf(seq, "test\t\t: yes\n");
	seq_printf(seq, "id\t\t: %d\n", plat_dev->id);

	return 0;
}

static int test_rtc_ioctl(struct device *dev, unsigned int cmd,
	unsigned long arg)
{
	/* We do support interrupts, they're generated
	 * using the sysfs interface.
	 */
	switch (cmd) {
	case RTC_PIE_ON:
	case RTC_PIE_OFF:
	case RTC_UIE_ON:
	case RTC_UIE_OFF:
	case RTC_AIE_ON:
	case RTC_AIE_OFF:
		return 0;

	default:
		return -ENOIOCTLCMD;
	}
}

static const struct rtc_class_ops test_rtc_ops = {
	.proc = test_rtc_proc,
	.read_time = test_rtc_read_time,
	.set_time = test_rtc_set_time,
	.read_alarm = test_rtc_read_alarm,
	.set_alarm = test_rtc_set_alarm,
	.set_mmss = test_rtc_set_mmss,
	.ioctl = test_rtc_ioctl,
};

static ssize_t test_irq_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", 42);
}
static ssize_t test_irq_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int retval;
	struct platform_device *plat_dev = to_platform_device(dev);
	struct rtc_device *rtc = platform_get_drvdata(plat_dev);

	retval = count;
	local_irq_disable();
	if (strncmp(buf, "tick", 4) == 0)
		rtc_update_irq(&rtc->class_dev, 1, RTC_PF | RTC_IRQF);
	else if (strncmp(buf, "alarm", 5) == 0)
		rtc_update_irq(&rtc->class_dev, 1, RTC_AF | RTC_IRQF);
	else if (strncmp(buf, "update", 6) == 0)
		rtc_update_irq(&rtc->class_dev, 1, RTC_UF | RTC_IRQF);
	else
		retval = -EINVAL;
	local_irq_enable();

	return retval;
}
static DEVICE_ATTR(irq, S_IRUGO | S_IWUSR, test_irq_show, test_irq_store);

static int test_probe(struct platform_device *plat_dev)
{
	int err;
	struct rtc_device *rtc = rtc_device_register("test", &plat_dev->dev,
						&test_rtc_ops, THIS_MODULE);
	if (IS_ERR(rtc)) {
		err = PTR_ERR(rtc);
		return err;
	}

	err = device_create_file(&plat_dev->dev, &dev_attr_irq);
	if (err)
		goto err;

	platform_set_drvdata(plat_dev, rtc);

	return 0;

err:
	rtc_device_unregister(rtc);
	return err;
}

static int __devexit test_remove(struct platform_device *plat_dev)
{
	struct rtc_device *rtc = platform_get_drvdata(plat_dev);

	rtc_device_unregister(rtc);
	device_remove_file(&plat_dev->dev, &dev_attr_irq);

	return 0;
}

static struct platform_driver test_drv = {
	.probe	= test_probe,
	.remove = __devexit_p(test_remove),
	.driver = {
		.name = "rtc-test",
		.owner = THIS_MODULE,
	},
};

static int __init test_init(void)
{
	int err;
	u8 data;
	u8 init_data[]={0x00,
					0x00,
					0x00,
					0x00,
					0x05,
					0x01,
					0x01,
					0x09,
					0x00,
					0x00,
					0x00,
					0x00,
					0x00,
					0x00,
					0x00,
					0x00,
					0x00,
					0x00,
					0x00,
					0x00,
					0x10
				};
	int i;				
	
	if ((err = platform_driver_register(&test_drv)))
		return err;

	if ((test0 = platform_device_alloc("rtc-test", 0)) == NULL) {
		err = -ENOMEM;
		goto exit_driver_unregister;
	}

	if ((test1 = platform_device_alloc("rtc-test", 1)) == NULL) {
		err = -ENOMEM;
		goto exit_free_test0;
	}

	if ((err = platform_device_add(test0)))
		goto exit_free_test1;

	if ((err = platform_device_add(test1)))
		goto exit_device_unregister;

	i2c_master_init();
	
	
	data = i2c_read_one_byte(0x01);
	
	
	if(data & 0x80){
		for(i=0; i<20; i++)
		{
			i2c_write(0x0, init_data, 20);	
		}
	}
//	data = 0x10;
//	i2c_write(0x13, &data, 1);
//
//	data = 0x00;
//	i2c_write(0x01, &data, 1);	//start up the rtc
//	
	data = 0x00;
	i2c_write(0x0C, &data, 1);	//start up the rtc
	
	return 0;

exit_device_unregister:
	platform_device_unregister(test0);

exit_free_test1:
	platform_device_put(test1);

exit_free_test0:
	platform_device_put(test0);

exit_driver_unregister:
	platform_driver_unregister(&test_drv);
	return err;
}

static void __exit test_exit(void)
{
	platform_device_unregister(test0);
	platform_device_unregister(test1);
	platform_driver_unregister(&test_drv);
}

MODULE_AUTHOR("Alessandro Zummo <a.zummo@towertech.it>");
MODULE_DESCRIPTION("RTC test driver/device");
MODULE_LICENSE("GPL");

module_init(test_init);
module_exit(test_exit);
