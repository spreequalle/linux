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
#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>   
#include <linux/fs.h>       
#include <linux/errno.h>    
#include <linux/types.h>    
#include <linux/proc_fs.h>
#include <linux/fcntl.h>    
#include <asm/system.h>     
#include <linux/wireless.h>

#include "i2c_drv.h"
#ifdef CONFIG_CAMEO_DAUGHTERBOARD
#include "ralink_gpio.h"
#endif

#ifdef  CONFIG_DEVFS_FS
#include <linux/devfs_fs_kernel.h>
#endif

#ifdef  CONFIG_DEVFS_FS
static devfs_handle_t devfs_handle;
#endif

int i2cdrv_major =  218;
unsigned long i2cdrv_addr = ATMEL_ADDR;
#ifdef CONFIG_CAMEO_DAUGHTERBOARD
#define SET 1
#define NOTSET 0
extern void ralink_gpio_setmod(int actflag, u32 mode_value);
#endif

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
/*           	i2c_WM8751_write                                               */
/*    INPUTS: 8-bit data                                                */
/*   RETURNS: None                                                      */
/*   OUTPUTS: None                                                      */
/*   NOTE(S): transfer 8-bit data to I2C                                */
/*                                                                      */
/*----------------------------------------------------------------------*/
#if defined (CONFIG_RALINK_I2S) || defined (CONFIG_RALINK_I2S_MODULE)
void i2c_WM8751_write(u32 address, u32 data)
{
	int i, j;
	unsigned long old_addr = i2cdrv_addr;
	
	i2cdrv_addr = WM8751_ADDR;
	
	i2c_master_init();	
	
	/* two bytes data at least so NODATA = 0 */

	RT2880_REG(RT2880_I2C_BYTECNT_REG) = 1;
	
	RT2880_REG(RT2880_I2C_DATAOUT_REG) = (address<<1)|(0x01&(data>>8));

	RT2880_REG(RT2880_I2C_STARTXFR_REG) = WRITE_CMD;

	j = 0;
	do {
		if (IS_SDOEMPTY) {	
			RT2880_REG(RT2880_I2C_DATAOUT_REG) = (data&0x0FF);	
			break;
		}
	} while (++j<max_ee_busy_loop);
	
	i = 0;
	while(IS_BUSY && i<i2c_busy_loop){
		i++;
	};
	i2cdrv_addr = old_addr;
}
#endif

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
static inline void random_read_block(u32 address, u8 *data)
{
	/* change page */
	if (ADDRESS_BYTES == 1) {
		int page;
		
		page = ((address >> 8) & 0x7) << 1;
		/* device id always 0 */
		RT2880_REG(RT2880_I2C_DEVADDR_REG) = (0xA0|page) >> 1;
	}

   	/* dummy write */
   	i2c_write(address, data, 0);
	i2c_read(data, READ_BLOCK);	
}

static inline u8 random_read_one_byte(u32 address)
{	
	u8 data;

	/* change page */
	if (ADDRESS_BYTES == 1) {
		int page;
		
		page = ((address >> 8) & 0x7) << 1;
		/* device id always 0 */
		RT2880_REG(RT2880_I2C_DEVADDR_REG) = (0xA0|page) >> 1;
	}


   	/* dummy write */
	i2c_write(address, &data, 0);
	i2c_read(&data, 1);
	return (data);
}

void i2c_eeprom_read(u32 address, u8 *data, u32 nbytes)
{
	int i;
	int nblock = nbytes / READ_BLOCK;
	int rem = nbytes % READ_BLOCK;

	for (i=0; i<nblock; i++) {
		random_read_block(address+i*READ_BLOCK, &data[i*READ_BLOCK]);
	}

	if (rem) {
		int offset = nblock*READ_BLOCK;
		for (i=0; i<rem; i++) {
			data[offset+i] = random_read_one_byte(address+offset+i);
		}		
	}
}


void i2c_eeprom_read_one(u32 address, u8 *data, u32 nbytes)
{
	int i;

	for (i=0; i<nbytes; i++) {
		data[i] = random_read_one_byte(address+i);
	}
}



static inline void random_write_block(u32 address, u8 *data)
{
	int i;
	/* change page */
	if (ADDRESS_BYTES == 1) {
		int page;
		
		page = ((address >> 8) & 0x7) << 1;
		/* device id always 0 */
		RT2880_REG(RT2880_I2C_DEVADDR_REG) = (0xA0|page) >> 1;
	}


	i2c_write(address, data, WRITE_BLOCK);
	// mdelay(5);
	for(i = 0; i < 500000; i++);
}

static inline void random_write_one_byte(u32 address, u8 *data)
{	
	int i;
	/* change page */
	if (ADDRESS_BYTES == 1) {
		int page;
		
		page = ((address >> 8) & 0x7) << 1;
		/* device id always 0 */
		RT2880_REG(RT2880_I2C_DEVADDR_REG) = (0xA0|page) >> 1;
	}

	i2c_write(address, data, 1);
	// mdelay(5);
	for(i = 0; i < 500000; i++);
}

void i2c_eeprom_write(u32 address, u8 *data, u32 nbytes)
{
	int i;
	int nblock = nbytes / WRITE_BLOCK;
	int rem = nbytes % WRITE_BLOCK;

	for (i=0; i<nblock; i++) {
		random_write_block(address+i*WRITE_BLOCK, &data[i*WRITE_BLOCK]);
	}

	if (rem) {
		int offset = nblock*WRITE_BLOCK;

		for (i=0; i<rem; i++) {
			random_write_one_byte(address+offset+i, &data[offset+i]);
		}		
	}
}



void i2c_read_config(char *data, unsigned int len)
{
	i2c_master_init();
	i2c_eeprom_read(0, data, len);
}
#ifdef CONFIG_CAMEO_DAUGHTERBOARD
#if defined (CONFIG_SO380K)
int i2c_SO380K_read_state(unsigned int* GpioState, unsigned int* BtnState)
{
	u8 Data[8];
	i2c_read(Data,8);
	if(GpioState)
		*GpioState = Data[4];
	if(BtnState)
		*BtnState = Data[6];

	printk("%x,%x\n",*GpioState, *BtnState);
	return;
}
	

int i2c_SO380K_read(unsigned int address, unsigned char* data)
{
	int i, j;
	unsigned char tmp;
	unsigned char d106[8];
	//send command
	RT2880_REG(RT2880_I2C_BYTECNT_REG) = 1;
	
	RT2880_REG(RT2880_I2C_DATAOUT_REG) =  address >> 8 ;// (address<<1)|(0x01&(data>>8));

	RT2880_REG(RT2880_I2C_STARTXFR_REG) = WRITE_CMD;
		
	j = 0;
	do {
		if (IS_SDOEMPTY) {
				RT2880_REG(RT2880_I2C_DATAOUT_REG) = address & 0xFF;
			break;
		}
	} while (++j<max_ee_busy_loop);


	i = 0;
	while(IS_BUSY && i<i2c_busy_loop){
		i++;
	};

	if(address == 0x106)
	{
		i2c_read(d106, 8);
		printk("%x,%x,%x,%x,%x,%x,%x,%x,\n",d106[0],d106[1],d106[2],d106[3],d106[4],d106[5],d106[6],d106[7]);
		
	}
	else
	{
	//read back
		i2c_read(data, 2);
	
	//swap data

		tmp = data[0] ;
		data[0]=data[1];
		data[1]=tmp;
	}
	return 0;
}

void i2c_SO380K_write(u32 address, u32 data)
{
	int i, j;

	u8 buf[4]; 

//   printk("[%s]:[%d]: ADDRESS_BYTES = %d\n", __FUNCTION__, __LINE__, ADDRESS_BYTES);
	
//	unsigned long old_addr = i2cdrv_addr;
	
//	i2cdrv_addr = SO380K_ADDR;
	
//	i2c_master_init();	
	
	/* two bytes data at least so NODATA = 0 */

	RT2880_REG(RT2880_I2C_BYTECNT_REG) = 3;
	
	RT2880_REG(RT2880_I2C_DATAOUT_REG) =  address >> 8 ;// (address<<1)|(0x01&(data>>8));

	RT2880_REG(RT2880_I2C_STARTXFR_REG) = WRITE_CMD;
	
	buf[0] = (u8)(data >> 8);
	buf[1] = (u8)(data & 0xFF);
	
	for (i=0; i<3; i++) {
		j = 0;
		do {
			if (IS_SDOEMPTY) {
				if (i==0) {
					RT2880_REG(RT2880_I2C_DATAOUT_REG) = address & 0xFF;
				} else {
					RT2880_REG(RT2880_I2C_DATAOUT_REG) = buf[i-1];
				}								
 			break;
			}
		} while (++j<max_ee_busy_loop);
//		printk("buf[%d] - %d",i,buf[i]);
	}


	i = 0;
	while(IS_BUSY && i<i2c_busy_loop){
		i++;
	};
//	i2cdrv_addr = old_addr;
}
#endif //(CONFIG_SO380K)

void so380k_set_init_table(void)
{

	I2C_WRITE init_table[] = {
	/* address, value, size */
		{0x0000, 0x0007, 2},
		{0x0001, 0x0020, 2},
		{0x0002, 0x0032, 2},
		{0x0004, 0x001F, 2},
		{0x0008, 0x0000, 2},
		{0x0010, 0xC0C0, 2},
		{0x0011, 0xC0C0, 2},
		{0x0012, 0x8EC0, 2},
		{0x0013, 0x9680, 2},
		{0x001E, 0x001F, 2},
		{0x0022, 0x001F, 2},
		{0x0023, 0x1818, 2},
		{0x0024, 0x1F1F, 2},
		{0x0025, 0x1F1F, 2},
		{0x0026, 0x001F, 2},
		{0x0027, 0x0000, 2},
		{0x002B, 0x0000, 2},
		{0x000E, 0x7F1F, 2},
		{0x000F, 0x0100, 2},//<<<<
		{0x0106, 0x0000, 2},
	};
	
	/* -- Writing Initial table-- */ 
	int i;
	I2C_WRITE *pData;
	printk("so380k set init table\n");
	for (i=0; i<20; i++){
		pData = (I2C_WRITE *) &init_table[i];
		i2c_SO380K_write(pData->address, pData->value);
		printk("addr: 0x%04X data: 0x%04X\n",pData->address, pData->value);
	}
	printk("------------------------\n");
} /* so380k_set_simple_init_table */


void so380k_init(void)
{	
	int j;
	//ATTNHigh();
//	ATTNLow();
	long data;
	//i2c_SO380K_write(0x0300,0x0001);
	for(j = 0; j < 500000; j++);
	//config gpio to i2c
	ralink_gpio_setmod(NOTSET, RALINK_GPIOMODE_I2C | RALINK_GPIOMODE_SPI);

	//init i2c interface
	i2cdrv_addr = SO380K_ADDR;
	
	i2c_master_init();	


	i2c_SO380K_write(0x0300,0x0001);

	for(j = 0; j < 500000; j++);

	// set Initialization table - from Reg.0x0000 to Reg.0x002B
	so380k_set_init_table();
	
	// set up read address from 0x0106 
	//so380k_set_read_addr();		
	i2c_SO380K_write(0x106, 0);
	
	i2c_SO380K_read(0x106,&data);
	i2c_SO380K_read(0x107,&data);
	i2c_SO380K_read(0x108,&data);
	i2c_SO380K_read(0x109,&data);

	// drop garbage
	//chk_key(1);
}
#endif

int i2cdrv_ioctl (struct inode *inode, struct file *filp, \
                     unsigned int cmd, unsigned long arg)
{
	unsigned char w_byte[4];
	int i;
	unsigned int address, size;
	unsigned long value;
	I2C_WRITE *i2c_write;


	switch (cmd) {
	case RT2880_I2C_READ:
		value = 0; address = 0;

		address = (unsigned int)arg;
		i2c_master_init();
		i2c_eeprom_read(address, (unsigned char*)&value, 4);
		printk("0x%04x : 0x%08x\n", address, value);
		break;
	case RT2880_I2C_WRITE:
		i2c_write = (I2C_WRITE*)arg;
		address = i2c_write->address;
		value   = i2c_write->value;
		size    = i2c_write->size;
		i2c_master_init();
		i2c_eeprom_write(address, (unsigned char*)&value, size);
#if 0
		memcpy(w_byte, (unsigned char*)&value, 4);
		if ( size == 4) {
			i2c_eeprom_write(address, w_byte[0], 1);
			i2c_eeprom_write(address+1, w_byte[1], 1 );
			i2c_eeprom_write(address+2, w_byte[2], 1 );
			i2c_eeprom_write(address+3, w_byte[3], 1 );
		} else if (size == 2) {
			i2c_eeprom_write(address, w_byte[0], 1);
			i2c_eeprom_write(address+1, w_byte[1], 1 );
		} else if (size == 1) {
			i2c_eeprom_write(address, w_byte[0], 1);
		} else {
			printk("i2c_drv -- size error, %d\n", size);
			return 0;
		}
#endif
		break;
#ifdef CONFIG_CAMEO_DAUGHTERBOARD
	case RT2880_I2C_KEYPAD_READ:
		value = 0; address = 0;

		address = *(unsigned int*) arg;				
		i2cdrv_addr = SO380K_ADDR;	
		i2c_master_init();
			
		i2c_SO380K_read(address, (unsigned char*)&value);

		//printk("read[%x] : %x", address, value);

		copy_to_user((char*)arg,(char*)&value,2);

		break;
	
	case RT2880_I2C_KEYPAD_WRITE:
		i2c_write = (I2C_WRITE*)arg;
		address = i2c_write->address;
		value   = i2c_write->value;
		size    = i2c_write->size;

		i2cdrv_addr = SO380K_ADDR;	
		i2c_master_init();			

		i2c_SO380K_write(address, value);

		break;
	case RT2880_I2C_KEYPAD_INIT:
		i2cdrv_addr = SO380K_ADDR;	
		i2c_master_init();			
		so380k_init();		

		break;
#endif
	default :
		printk("i2c_drv: command format error\n");
	}

	return 0;
}

struct file_operations i2cdrv_fops = {
    ioctl:      i2cdrv_ioctl,
};

static int i2cdrv_init(void)
{

#ifdef  CONFIG_DEVFS_FS
    if(devfs_register_chrdev(i2cdrv_major, I2C_DEV_NAME , &i2cdrv_fops)) {
	printk(KERN_WARNING " i2cdrv: can't create device node\n");
	return -EIO;
    }

    devfs_handle = devfs_register(NULL, I2C_DEV_NAME, DEVFS_FL_DEFAULT, i2cdrv_major, 0, \
				S_IFCHR | S_IRUGO | S_IWUGO, &i2cdrv_fops, NULL);
#else
    int result=0;
    result = register_chrdev(i2cdrv_major, I2C_DEV_NAME, &i2cdrv_fops);
    if (result < 0) {
        printk(KERN_WARNING "i2c_drv: can't get major %d\n",i2cdrv_major);
        return result;
    }

    if (i2cdrv_major == 0) {
			i2cdrv_major = result; /* dynamic */
    }
#endif

    printk("i2cdrv_major = %d\n", i2cdrv_major);
    return 0;

}



static void i2cdrv_exit(void)
{
    printk("i2c_drv exit\n");

#ifdef  CONFIG_DEVFS_FS
    devfs_unregister_chrdev(i2cdrv_major, I2C_DEV_NAME);
    devfs_unregister(devfs_handle);
#else
    unregister_chrdev(i2cdrv_major, I2C_DEV_NAME);
#endif

}

#if defined (CONFIG_RALINK_I2S) || defined (CONFIG_RALINK_I2S_MODULE)
EXPORT_SYMBOL(i2c_WM8751_write);
#endif

#ifdef CONFIG_CAMEO_DAUGHTERBOARD
#if defined (CONFIG_SO380K)
EXPORT_SYMBOL(i2c_SO380K_write);
#endif
#endif

module_init(i2cdrv_init);
module_exit(i2cdrv_exit);

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,12)
MODULE_PARM (i2cdrv_major, "i");
#else
module_param (i2cdrv_major, int, 0);
#endif

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Ralink I2C Driver");

