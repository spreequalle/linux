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

#include "spi_drv.h"
#include "ralink_gpio.h"

#if defined (CONFIG_MAC_TO_MAC_MODE) || defined (CONFIG_P5_RGMII_TO_MAC_MODE)
#include <linux/delay.h>
#include "vtss.h"
#endif

#ifdef  CONFIG_DEVFS_FS
#include <linux/devfs_fs_kernel.h>
#endif

#ifdef  CONFIG_DEVFS_FS
static devfs_handle_t devfs_handle;
#endif
int spidrv_major =  217;

/*----------------------------------------------------------------------*/
/*   Function                                                           */
/*           	spi_chip_select                                         */
/*    INPUTS: ENABLE or DISABLE                                         */
/*   RETURNS: None                                                      */
/*   OUTPUTS: None                                                      */
/*   NOTE(S): Pull on or Pull down #CS                                  */
/*                                                                      */
/*----------------------------------------------------------------------*/

static inline void spi_chip_select(u8 enable)
{
	int i;

	for (i=0; i<spi_busy_loop; i++) {
		if (!IS_BUSY) {
			/* low active */
			if (enable) {		
				RT2880_REG(RT2880_SPICTL_REG) =	SPICTL_SPIENA_ON;
			} else  {
				RT2880_REG(RT2880_SPICTL_REG) = SPICTL_SPIENA_OFF;
			}		
			break;
		}
	}

#ifdef DBG
	if (i == spi_busy_loop) {
		printk("warning : spi_transfer (spi_chip_select) busy !\n");
	}
#endif
}

/*----------------------------------------------------------------------*/
/*   Function                                                           */
/*           	spi_master_init                                         */
/*    INPUTS: None                                                      */
/*   RETURNS: None                                                      */
/*   OUTPUTS: None                                                      */
/*   NOTE(S): Initialize SPI block to desired state                     */
/*                                                                      */
/*----------------------------------------------------------------------*/
void spi_master_init(void)
{
	int i;
	/* reset spi block */
	RT2880_REG(RT2880_RSTCTRL_REG) |= RSTCTRL_SPI_RESET;
	RT2880_REG(RT2880_RSTCTRL_REG) &= ~(RSTCTRL_SPI_RESET);
	/* udelay(500); */
	for ( i = 0; i < 1000; i++);
	

	//RT2880_REG(RT2880_SPICFG_REG) = SPICFG_MSBFIRST | 
	//								SPICFG_RXCLKEDGE_FALLING |
	//								SPICFG_TXCLKEDGE_FALLING |
	//								SPICFG_SPICLK_DIV128;
	RT2880_REG(RT2880_SPICFG_REG) = SPICFG_MSBFIRST | 
									SPICFG_RXCLKEDGE_FALLING |
									SPICFG_TXCLKEDGE_FALLING |
									SPICFG_SPICLK_DIV128;	
	spi_chip_select(DISABLE);

#ifdef DBG
	printk("SPICFG = %08x\n", RT2880_REG(RT2880_SPICFG_REG));
	printk("is busy %d\n", IS_BUSY);
#endif		 	
}
#ifdef CONFIG_CAMEO_DAUGHTERBOARD
void spi_cameo_master_init(void)
{
	int i;
	/* reset spi block */
	RT2880_REG(RT2880_RSTCTRL_REG) |= RSTCTRL_SPI_RESET;
	RT2880_REG(RT2880_RSTCTRL_REG) &= ~(RSTCTRL_SPI_RESET);
	/* udelay(500); */
	for ( i = 0; i < 1000; i++);
	
	RT2880_REG(RT2880_SPICFG_REG) = SPICFG_MSBFIRST |
									SPICFG_SPICLKPOL |
									SPICFG_RXCLKEDGE_FALLING |
									SPICFG_TXCLKEDGE_FALLING |
//									SPICFG_RXCLKEDGE_RAISING |
//									SPICFG_TXCLKEDGE_RAISING |
									SPICFG_HIZSIP |
									SPICFG_SPICLK_DIV32;	

	spi_chip_select(DISABLE);

#ifdef DBG
	printk("SPICFG = %08x\n", RT2880_REG(RT2880_SPICFG_REG));
	printk("is busy %d\n", IS_BUSY);
#endif		 	


}

void spi_5406b_master_init(void)
{
	int i;
	/* reset spi block */
	RT2880_REG(RT2880_RSTCTRL_REG) |= RSTCTRL_SPI_RESET;
	RT2880_REG(RT2880_RSTCTRL_REG) &= ~(RSTCTRL_SPI_RESET);
	/* udelay(500); */
	for ( i = 0; i < 1000; i++);
	
	RT2880_REG(RT2880_SPICFG_REG) = SPICFG_MSBFIRST |
									SPICFG_SPICLKPOL |
//									SPICFG_RXCLKEDGE_FALLING |
//									SPICFG_TXCLKEDGE_FALLING |
									SPICFG_RXCLKEDGE_RAISING |
									SPICFG_TXCLKEDGE_RAISING |
									SPICFG_HIZSIP |
									SPICFG_SPICLK_DIV128;	

	spi_chip_select(DISABLE);

#ifdef DBG
	printk("SPICFG = %08x\n", RT2880_REG(RT2880_SPICFG_REG));
	printk("is busy %d\n", IS_BUSY);
#endif		 	


}

#endif

/*----------------------------------------------------------------------*/
/*   Function                                                           */
/*           	spi_write                                               */
/*    INPUTS: 8-bit data                                                */
/*   RETURNS: None                                                      */
/*   OUTPUTS: None                                                      */
/*   NOTE(S): transfer 8-bit data to SPI                                */
/*                                                                      */
/*----------------------------------------------------------------------*/
static void spi_write(u8 data)
{
	int i;

	for (i=0; i<spi_busy_loop; i++) {
		if (!IS_BUSY) {
			RT2880_REG(RT2880_SPIDATA_REG) = data;
			/* start write transfer */
			RT2880_REG(RT2880_SPICTL_REG) = SPICTL_HIZSDO |  SPICTL_STARTWR | 
											SPICTL_SPIENA_ON;
			break;
		}
	}

#ifdef DBG
	if (i == spi_busy_loop) {
		printk("warning : spi_transfer (write %02x) busy !\n", data);
	}
#endif
}

#if defined (CONFIG_MAC_TO_MAC_MODE) || defined (CONFIG_P5_RGMII_TO_MAC_MODE)
//write32 MSB first
static void spi_write32(u32 data)
{
	u8 d0, d1, d2, d3;

	d0 = (u8)((data >> 24) & 0xff);
	d1 = (u8)((data >> 16) & 0xff);
	d2 = (u8)((data >> 8) & 0xff);
	d3 = (u8)(data & 0xff);

	spi_write(d0);
	spi_write(d1);
	spi_write(d2);
	spi_write(d3);
}
#endif

/*----------------------------------------------------------------------*/
/*   Function                                                           */
/*           	spi_read                                                */
/*    INPUTS: None                                                      */
/*   RETURNS: 8-bit data                                                */
/*   OUTPUTS: None                                                      */
/*   NOTE(S): get 8-bit data from SPI                                   */
/*                                                                      */
/*----------------------------------------------------------------------*/
static u8 spi_read(void) 
{
	int i;

	/* 
	 * poll busy bit until it is 0 
	 * then start read transfer
	 */
	for (i=0; i<spi_busy_loop; i++) {
		if (!IS_BUSY) {
			RT2880_REG(RT2880_SPIDATA_REG) = 0;
			/* start read transfer */
			RT2880_REG(RT2880_SPICTL_REG) = SPICTL_STARTRD |
											SPICTL_SPIENA_ON;
			break;
		}
	}

	/* 
	 * poll busy bit until it is 0 
	 * then get data 
	 */
	for (i=0; i<spi_busy_loop; i++) {
		if (!IS_BUSY) {
			break;
		}
	}
	
#ifdef DBG		
	if (i == spi_busy_loop) {
		printk("warning : spi_transfer busy !\n");
	} 
#endif

	return ((u8)RT2880_REG(RT2880_SPIDATA_REG));
}

#if defined (CONFIG_MAC_TO_MAC_MODE) || defined (CONFIG_P5_RGMII_TO_MAC_MODE)
//read32 MSB first
static u32 spi_read32(void)
{
	u8 d0, d1, d2, d3;
	u32 ret;

	d0 = spi_read();
	d1 = spi_read();
	d2 = spi_read();
	d3 = spi_read();
	ret = (d0 << 24) | (d1 << 16) | (d2 << 8) | d3;

	return ret;
}
#endif

/*----------------------------------------------------------------------*/
/*   Function                                                           */
/*           	eeprom_get_status_reg                                   */
/*    INPUTS: pointer to status                                         */
/*   RETURNS: None                                                      */
/*   OUTPUTS: status                                                    */
/*   NOTE(S): get the status of eeprom (AT25xxxx)                       */
/*                                                                      */
/*----------------------------------------------------------------------*/
static void eeprom_get_status_reg(u8 *status) 
{
	spi_chip_select(ENABLE);
	spi_write(RDSR_CMD);
	*status = spi_read();		
	spi_chip_select(DISABLE);
}


#ifdef CONFIG_CAMEO_DAUGHTERBOARD
u8 spi_cameo_read8(unsigned char reg)
{
	int i;
	u8 value1=0,value2=0,value3=0;
	//set bit7
	reg |= (1<<7);
	spi_chip_select(ENABLE);
	spi_write(reg);
	i=1000;
	while(i--);	//delay.

	value1 = spi_read();
	spi_chip_select(DISABLE);

	//printk(" spi cameo read  %x,%x,%x\n",value1,value2,value3);

	return value1;
}
unsigned char spi_cameo_read(u16 address, u8 *dest)
{
	u8 reg;
	reg = address;
	*dest = spi_cameo_read8(address);
	return 0;
}

void spi_5406b_read16( unsigned char cmd, unsigned int value)
{

	int value16;

	u8 StartByte, HiByte, LowByte;
	if(cmd == 1)
		 StartByte = 0x70 | 0x03;  //write data;
	else
		 StartByte = 0x70  | 0x01;	

	HiByte = (u8) (value >> 8);
	LowByte = (u8) (value & 0x00FF);

//	spi_5406b_master_init();
	spi_chip_select(ENABLE);
	spi_write(StartByte);
	HiByte = spi_read();
	LowByte = spi_read();
	spi_chip_select(DISABLE);

	value16 = HiByte;
	value16 = value16 << 8;
	value16 = value16 | LowByte;

	printk("value : %x\n",value16);
}

void spi_5406b_write16( unsigned char cmd, unsigned int value)
{
	int i; 
	int delay_count;

	u8 StartByte, HiByte, LowByte;
	if(cmd == 1)
		 StartByte = 0x70 | 0x02;  //write data;
	else
		 StartByte = 0x70;	

	HiByte = (u8) (value >> 8);
	LowByte = (u8) (value & 0x00FF);

//	spi_5406b_master_init();
	spi_chip_select(ENABLE);
	spi_write(StartByte);
	spi_write(HiByte);
	spi_write(LowByte);
	spi_chip_select(DISABLE);
	delay_count = CAMEO_DELAY;
	while(delay_count--);
}

unsigned char spi_cameo_write8(u8 reg, u8 value)
{

	int delay_index;
	//clear bit7
	reg &=  ~(1<<7);
	
	spi_chip_select(ENABLE);
	spi_write(reg);
	spi_chip_select(DISABLE);
//	delay_index=500;
//	while(delay_index--);
	spi_chip_select(ENABLE);
	spi_write(value);
	spi_chip_select(DISABLE);	
	//for delay
	delay_index=CAMEO_DELAY;
	while(delay_index--);
	
	//printk("reg[%x] : %x\n",reg, value);
	return 0;
}
void spi_cameo_lcd_on(int on)
{
	int delay_index1;
	printk("spi_cameo_lcd_on %d\n",on);
	
		spi_chip_select(ENABLE);
		spi_write(0x08);
		spi_chip_select(DISABLE);
		delay_index1=CAMEO_DELAY;
		while(delay_index1--);

	if(on){
		spi_chip_select(ENABLE);
		spi_write(0x6f);
		spi_chip_select(DISABLE);
		delay_index1=CAMEO_DELAY;
		while(delay_index1--);
		spi_chip_select(ENABLE);
		spi_write(0x6e);
		spi_chip_select(DISABLE);
		delay_index1=CAMEO_DELAY;
		while(delay_index1--);
		spi_chip_select(ENABLE);
		spi_write(0x6e);
		spi_chip_select(DISABLE);
		delay_index1=CAMEO_DELAY;
		while(delay_index1--);
		spi_chip_select(ENABLE);
		spi_write(0x6e);
		spi_chip_select(DISABLE);
		delay_index1=CAMEO_DELAY;
		while(delay_index1--);
		spi_chip_select(ENABLE);
		spi_write(0x6e);
		spi_chip_select(DISABLE);
		delay_index1=CAMEO_DELAY;
		while(delay_index1--);
	
	}
	else{
		spi_chip_select(ENABLE);
		spi_write(0x6f);
		spi_chip_select(DISABLE);
		delay_index1=CAMEO_DELAY;
		while(delay_index1--);
		spi_chip_select(ENABLE);
		spi_write(0x66);
		spi_chip_select(DISABLE);
		delay_index1=CAMEO_DELAY;
		while(delay_index1--);
		spi_chip_select(ENABLE);
		spi_write(0x66);
		spi_chip_select(DISABLE);
		delay_index1=CAMEO_DELAY;
		while(delay_index1--);		
		spi_chip_select(ENABLE);
		spi_write(0x66);
		spi_chip_select(DISABLE);
		delay_index1=CAMEO_DELAY;
		while(delay_index1--);		
		spi_chip_select(ENABLE);
		spi_write(0x66);
		spi_chip_select(DISABLE);
		delay_index1=CAMEO_DELAY;
		while(delay_index1--);
	}	
	
}
void spi_cameo_reset_pic(void)
{
		int delay_index1;

		spi_chip_select(ENABLE);
		spi_write(0x08);
		spi_chip_select(DISABLE);
		delay_index1=CAMEO_DELAY;
		while(delay_index1--);
		spi_chip_select(ENABLE);
		spi_write(0x72);
		spi_chip_select(DISABLE);
		delay_index1=CAMEO_DELAY;
		while(delay_index1--);
		spi_chip_select(ENABLE);
		spi_write(0x65);
		spi_chip_select(DISABLE);
		delay_index1=CAMEO_DELAY;
		while(delay_index1--);
		spi_chip_select(ENABLE);
		spi_write(0x73);
		spi_chip_select(DISABLE);
		delay_index1=CAMEO_DELAY;
		while(delay_index1--);
		spi_chip_select(ENABLE);
		spi_write(0x65);
		spi_chip_select(DISABLE);
		delay_index1=CAMEO_DELAY;
		while(delay_index1--);
		spi_chip_select(ENABLE);
		spi_write(0x74);
		spi_chip_select(DISABLE);
		delay_index1=CAMEO_DELAY;
		while(delay_index1--);


}
void spi_cameo_write_linear(void)
{
	int ii;
	int delay_index1;

	u16 iSize = 220*176;
	u8 SizeHibyte, SizeLobyte;

	u8 Data[2] = {0x03,0xFF};

	SizeHibyte = (iSize >> 8) & 0xFF;
	SizeLobyte = iSize & 0xFF;
	
	spi_cameo_write8(4, SizeHibyte);
	spi_cameo_write8(5, SizeLobyte);	
	spi_cameo_write8(7, 1);	

	spi_cameo_write8(1, 0x36);
	spi_cameo_write8(2, 0xA0);
	spi_cameo_write8(1, 0x2A);
	spi_cameo_write8(2, 0);
	spi_cameo_write8(2, 0);
	spi_cameo_write8(2, 0);
	spi_cameo_write8(2, 219);
	spi_cameo_write8(1, 0x2B);
	spi_cameo_write8(2, 0);
	spi_cameo_write8(2, 0);
	spi_cameo_write8(2, 0);
	spi_cameo_write8(2, 175);
	spi_cameo_write8(1, 0x2C);

   spi_chip_select(ENABLE);
	spi_write(6);
	spi_chip_select(DISABLE);	
		delay_index1=CAMEO_DELAY;
		while(delay_index1--);

	for(ii=0; ii< iSize; ii++)
	{

		spi_chip_select(ENABLE);
		spi_write(0x03);
		spi_chip_select(DISABLE);
		delay_index1=CAMEO_DELAY;
		while(delay_index1--);
		spi_chip_select(ENABLE);
		spi_write(0xf0);
		spi_chip_select(DISABLE);	

		delay_index1=CAMEO_DELAY;
		while(delay_index1--);

	}
	printk("size = %x\n",iSize);

}
#endif
/*----------------------------------------------------------------------*/
/*   Function                                                           */
/*           	eeprom_read                                             */
/*    INPUTS: address - start address to be read                        */ 
/*            nbytes  - number of bytes to be read                      */
/*            dest    - pointer to read buffer                          */
/*   RETURNS: 0 - successful                                            */
/*            or eeprom status                                          */
/*   OUTPUTS: read buffer                                               */
/*   NOTE(S): If the eeprom is busy , the function returns with status  */
/*            register of eeprom                                        */
/*----------------------------------------------------------------------*/
unsigned char spi_eeprom_read(u16 address, u16 nbytes, u8 *dest)
{
	u8	status;
	u16	cnt = 0;
	int i = 0;

	do {
		i++;
		eeprom_get_status_reg(&status);		
	}
	while((status & (1<<RDY)) && (i < max_ee_busy_loop));

	if (i == max_ee_busy_loop)
		return (status);

	/* eeprom ready */
	if (!(status & (1<<RDY))) {	
		spi_chip_select(ENABLE);
		/* read op */
		spi_write(READ_CMD);
		spi_write((u8)(address >> 8));		/* MSB byte First */
		spi_write((u8)(address & 0x00FF));	/* LSB byte */

		while (cnt < nbytes) {
			*(dest++) = spi_read();
			cnt++;
		}
		status = 0;
		/* deassert cs */
		spi_chip_select(DISABLE);
	} 
	return (status);	
}


/*----------------------------------------------------------------------*/
/*   Function                                                           */
/*           	eeprom_write_enable                                     */
/*    INPUTS: None                                                      */
/*   RETURNS: None                                                      */
/*   OUTPUTS: None                                                      */
/*   NOTE(S): always perform write enable  before any write operation   */
/*                                                                      */
/*----------------------------------------------------------------------*/
static void eeprom_write_enable(void)
{
	unsigned char	status;
	int i = 0;

	do {
		i++;
		eeprom_get_status_reg(&status);		
	}
	while((status & (1<<RDY)) && (i < max_ee_busy_loop));

	if (i == max_ee_busy_loop)
		return;

	/* eeprom ready */
	if (!(status & (1<<RDY))) 
	{	
		spi_chip_select(ENABLE);
		/* always write enable  before any write operation */
		spi_write(WREN_CMD);

		spi_chip_select(DISABLE);
		
		/* wait for write enable */
		do {
			eeprom_get_status_reg(&status);
		} while((status & (1<<RDY)) || !(status & (1<<WEN)));

	}

}

/*----------------------------------------------------------------------*/
/*   Function                                                           */
/*           	eeprom_write                                            */
/*    INPUTS: address - the first byte address to be written            */
/*            nbytes  - the number of bytes to be written               */
/*            src     - the pointer to source buffer                    */     
/*   RETURNS: 0  - successful                                           */
/*            or eeprom buy status                                      */
/*   OUTPUTS: None                                                      */
/*   NOTE(S): The different eeprom has various write page size          */
/* 			  The function don't care write page size so the caller     */
/*            must check the page size of eeprom                        */
/*                                                                      */
/*----------------------------------------------------------------------*/
unsigned char spi_eeprom_write(u16 address, u16 nbytes, u8 *src)
{
	unsigned char	status;
	unsigned int	cnt = 0;
	int i = 0;

	do {
		i++;
		eeprom_get_status_reg(&status);		
	}
	while((status & (1<<RDY)) && (i < max_ee_busy_loop));

	if (i == max_ee_busy_loop)
		goto done;


	/* eeprom ready */
	if (!(status & (1<<RDY))) {			
		/* always write enable  before any write operation */		
		eeprom_write_enable();

		spi_chip_select(ENABLE);
		spi_write(WRITE_CMD);
		spi_write((u8)(address >> 8));		/* MSB byte First */
		spi_write((u8)(address & 0x00FF));	/* LSB byte */

		while (cnt < nbytes) {
			spi_write(src[cnt]);
			cnt++;
		}
		status = 0;
		/* last byte sent then pull #cs high  */
		spi_chip_select(DISABLE);
	} 

	i = 0;
	do {
		i++;
		eeprom_get_status_reg(&status);		
	}
	while((status & (1<<RDY)) && (i < max_ee_busy_loop));

done:
	return (status);
}

#if defined (CONFIG_MAC_TO_MAC_MODE) || defined (CONFIG_P5_RGMII_TO_MAC_MODE)
void spi_vtss_read(u8 blk, u8 subblk, u8 addr, u32 *value)
{
	u8 cmd;

	spi_master_init();
	cmd = (u8)((blk << 5) | subblk);
	spi_write(cmd);
	spi_write(addr);
	spi_read(); //dummy byte
	spi_read(); //dummy byte
	*value = spi_read32();
	//printf("rd %x:%x:%x = %x\n", blk, subblk, addr, *value);
	udelay(100);
}

void spi_vtss_write(u8 blk, u8 subblk, u8 addr, u32 value)
{
	u8 cmd;

	spi_master_init();
	cmd = (u8)((blk << 5) | subblk | 0x10);
	spi_write(cmd);
	spi_write(addr);
	spi_write32(value);
	//printf("wr %x:%x:%x = %x\n", blk, subblk, addr, value);
	udelay(10);
}

void vtss_reset(void)
{
#ifdef CONFIG_RT2883_ASIC
	RT2880_REG(RALINK_REG_GPIOMODE) |= (7 << 2);
	RT2880_REG(RALINK_REG_GPIOMODE) &= ~(1 << 1);

	RT2880_REG(RALINK_REG_PIODIR) |= (1 << 12);
	RT2880_REG(RALINK_REG_PIODIR) &= ~(1 << 7);

	//Set Gpio pin 12 to low
	RT2880_REG(RALINK_REG_PIODATA) &= ~(1 << 12);

	mdelay(50);
	//Set Gpio pin 12 to high
	RT2880_REG(RALINK_REG_PIODATA) |= (1 << 12);

	mdelay(125);
#elif defined CONFIG_RT3052_ASIC
	RT2880_REG(RALINK_REG_GPIOMODE) |= (7 << 2);
	RT2880_REG(RALINK_REG_GPIOMODE) &= ~(1 << 1);

	RT2880_REG(RALINK_REG_PIODIR) |= (1 << 12);
	RT2880_REG(RALINK_REG_PIODIR) &= ~(1 << 7);

	//Set Gpio pin 36 to low
	RT2880_REG(RALINK_REG_PIO3924DATA) &= ~(1 << 12);

	mdelay(50);
	//Set Gpio pin 36 to high
	RT2880_REG(RALINK_REG_PIO3924DATA) |= (1 << 12);

	mdelay(125);
#else
	RT2880_REG(RALINK_REG_GPIOMODE) |= (1 << 1);

	RT2880_REG(RALINK_REG_PIODIR) |= (1 << 10);

	//Set Gpio pin 10 to low
	RT2880_REG(RALINK_REG_PIODATA) &= ~(1 << 10);

	mdelay(50);
	//Set Gpio pin 10 to high
	RT2880_REG(RALINK_REG_PIODATA) |= (1 << 10);

	mdelay(125);
#endif
}

//type 0: no vlan, 1: vlan
void vtss_init(int type)
{
	int i, len;
	u32 tmp;

	len = (type == 0)? sizeof(lutonu_novlan) : sizeof(lutonu_vlan);

	//HT_WR(SYSTEM, 0, ICPU_CTRL, (1<<7) | (1<<3) | (1<<2) | (0<<0));
	//read it out to be sure the reset was done.
	while (1) {
		spi_vtss_write(7, 0, 0x10, (1<<7) | (1<<3) | (1<<2) | (0<<0));
		spi_vtss_read(7, 0, 0x10, &tmp);
		if (tmp & ((1<<7) | (1<<3) | (1<<2) | (0<<0)))
			break;
		udelay(1000);
	}

	//HT_WR(SYSTEM, 0, ICPU_ADDR, 0); //clear SP_SELECT and ADDR
	spi_vtss_write(7, 0, 0x11, 0);

	if (type == 0) {
		for (i = 0; i < len; i++) {
			//HT_WR(SYSTEM, 0, ICPU_DATA, lutonu[i]);
			spi_vtss_write(7, 0, 0x12, lutonu_novlan[i]);
		}
	}
	else {
		for (i = 0; i < len; i++) {
			//HT_WR(SYSTEM, 0, ICPU_DATA, lutonu[i]);
			spi_vtss_write(7, 0, 0x12, lutonu_vlan[i]);
		}
	}

	//debug
	/*
	spi_vtss_write(7, 0, 0x11, 0);
	spi_vtss_read(7, 0, 0x11, &tmp);
	printk("ICPU_ADDR %x\n", tmp);
	for (i = 0; i < len; i++) {
		spi_vtss_read(7, 0, 0x12, &tmp);
		printk("%x ", tmp);
	}
	*/

	//HT_WR(SYSTEM, 0, GLORESET, (1<<0)); //MASTER_RESET
	spi_vtss_write(7, 0, 0x14, (1<<0));
	udelay(125000);

	//HT_WR(SYSTEM, 0, ICPU_CTRL, (1<<8) | (1<<3) | (1<<1) | (1<<0));
	spi_vtss_write(7, 0, 0x10, (1<<8) | (1<<3) | (1<<1) | (1<<0));
}
#endif

#ifdef CONFIG_CAMEO_DAUGHTERBOARD
void CountICONSize(unsigned int size, unsigned char *byte0, unsigned char *byte1) {
     int temp;
       
 	  temp = size & 0xff00;
	  *byte0 = (unsigned char) (temp >> 8);
	  temp = size & 0x00ff;
	  *byte1 = (unsigned char) temp;
              	
}	

unsigned char show_image_command(int sX, int eX, int sY, int eY, int size, unsigned int imagepage)
{
	u8 s0, s1;

	CountICONSize(size, &s0, &s1);
	//printk("%x,%x,%x,%x,%x,%x,%x\n",sX,eX,sY,eY,s0,s1,imagepage);	
	spi_chip_select(ENABLE);
	spi_write(48);//SPI_COMMAND_SHOW_IMAGE);
	spi_write((u8)sX);
	spi_write((u8)eX);
	spi_write((u8)sY);
	spi_write((u8)eY);
	spi_write(s0);
	spi_write(s1);
	spi_write((u8)(imagepage>>8));
	spi_write((u8)(imagepage&0x00FF));	
	spi_chip_select(DISABLE);
//	mdelay(DELAY_DRAW2);
	return 0;
}


void get_picversion(void)
{
	unsigned char MajVer, MinVer, ChkSumHi, ChkSumLo;
	
	spi_chip_select(ENABLE);

	spi_write(55);
	MajVer = spi_read();
	spi_write(56);
	MinVer = spi_read();
	spi_write(57);
	ChkSumHi = spi_read();
	spi_write(58);
	ChkSumLo = spi_read();
//	ChkSumLo = spi_read();

	spi_chip_select(DISABLE);	

	//printk("Picversion : v%02x.%02x.%02x%02x\n", MajVer, MinVer, ChkSumHi, ChkSumLo);

}

void spi_cameo_lcd(unsigned int lcd_cmd)
{
	unsigned char ChkSumLo;
	int i;
//	unsigned char data[220*176*2];
	switch(lcd_cmd)
	{
	case 0: //clean
		show_image_command(0, 219, 0, 175, 220*176*2, 0xffff);
		break;

	case 1: //read version
		get_picversion();
		break;
	case 2:
		show_image_command(191, 207, 5, 17,  17*13*2, 262);
		break;
	case 3:
		spi_chip_select(ENABLE);	
		spi_write(0);
		spi_chip_select(DISABLE);	
		break;
	case 4:
		spi_chip_select(ENABLE);	
		spi_write(30);
		spi_chip_select(DISABLE);	
		break;
	case 5:
		spi_chip_select(ENABLE);	
		spi_write(55);
		spi_chip_select(DISABLE);
		i=8000;
		while(i--);

		spi_chip_select(ENABLE);	
		ChkSumLo = spi_read();
		spi_chip_select(DISABLE);	
		printk("Picversion : %02x\n", ChkSumLo);
		break;
	case 6:
		spi_chip_select(ENABLE);	
		spi_write(56);
		spi_chip_select(DISABLE);
		spi_chip_select(ENABLE);	
		ChkSumLo = spi_read();
		spi_chip_select(DISABLE);	
		printk("Picversion : %02x\n", ChkSumLo);
		break;
	case 7:
		spi_chip_select(ENABLE);	
		spi_write(57);
		spi_chip_select(DISABLE);
		spi_chip_select(ENABLE);	
		ChkSumLo = spi_read();
		spi_chip_select(DISABLE);	
		printk("Picversion : %02x\n", ChkSumLo);
		break;
	case 8:
		spi_cameo_write8(1,0x36);
		spi_cameo_write8(2,0xA0);
		printk("test spi cameo write 8 command \n");
		break;

	 case 9:

		spi_cameo_write8(1,0x36);
		spi_cameo_write8(2,0xA0);
		spi_cameo_write8(1, 0x2A);
		spi_cameo_write8(2, 0);
		spi_cameo_write8(2, 0);
		spi_cameo_write8(2, 0);
		spi_cameo_write8(2, 19);
		spi_cameo_write8(1, 0x2B);
		spi_cameo_write8(2, 0);
		spi_cameo_write8(2, 0);
		spi_cameo_write8(2, 0);
		spi_cameo_write8(2, 19);
		spi_cameo_write8(1, 0x2C);
		i=20*20;
		while(i--){
			spi_cameo_write8(2, 0x03);	
			spi_cameo_write8(2, 0xFF);
		}
		break;
	 case 10:

		spi_cameo_write8(1,0x36);
		spi_cameo_write8(2,0xA0);
		spi_cameo_write8(1, 0x2A);
		spi_cameo_write8(2, 0);
		spi_cameo_write8(2, 0);
		spi_cameo_write8(2, 0);
		spi_cameo_write8(2, 219);
		spi_cameo_write8(1, 0x2B);
		spi_cameo_write8(2, 0);
		spi_cameo_write8(2, 0);
		spi_cameo_write8(2, 0);
		spi_cameo_write8(2, 175);
		spi_cameo_write8(1, 0x2C);
		i=220*176;
		while(i--){
			spi_cameo_write8(2, 0x0);	
			spi_cameo_write8(2, 0x0);
		}
		break;

	 case 11:

		spi_cameo_write8(1,0x36);
		spi_cameo_write8(2,0xA0);
		spi_cameo_write8(1, 0x2A);
		spi_cameo_write8(2, 0);
		spi_cameo_write8(2, 0);
		spi_cameo_write8(2, 0);
		spi_cameo_write8(2, 219);
		spi_cameo_write8(1, 0x2B);
		spi_cameo_write8(2, 0);
		spi_cameo_write8(2, 0);
		spi_cameo_write8(2, 0);
		spi_cameo_write8(2, 175);
		spi_cameo_write8(1, 0x2C);
		i=220*176;
		while(i--){
			spi_cameo_write8(2, 0xFF);	
			spi_cameo_write8(2, 0xFF);
		}
		break;
	  
		case 12:
		spi_cameo_write_linear();
		break;
	
		case 13:
		printk("spi_cameo_reset_pic \n");
		spi_cameo_reset_pic();
		printk("spi_cameo_reset_pic \n");
		break;

		case 14:
		printk("init 5406\n");
		spi_5406b_write16(0,0x0000);
		spi_5406b_write16(1,0x0000);
		spi_5406b_write16(0,0x0001);
		spi_5406b_write16(1,0x031C);
		spi_5406b_write16(0,0x0002);
		spi_5406b_write16(1,0x0100);
		spi_5406b_write16(0,0x0003);
		spi_5406b_write16(1,0x1038);
		spi_5406b_write16(0,0x000F);
		spi_5406b_write16(1,0x1401);
		spi_5406b_write16(0,0x000B);
		spi_5406b_write16(1,0x0000);
		spi_5406b_write16(0,0x0010);
		spi_5406b_write16(1,0x0200);
		spi_5406b_write16(0,0x0012);
		spi_5406b_write16(1,0x2111);
		spi_5406b_write16(0,0x0013);
		spi_5406b_write16(1,0x006D);
		spi_5406b_write16(0,0x0014);
		spi_5406b_write16(1,0x4168);
		spi_5406b_write16(0,0x0015);
		spi_5406b_write16(1,0x0000);

		spi_5406b_write16(0,0x0050);
		spi_5406b_write16(1,0x0501);
		spi_5406b_write16(0,0x0051);
		spi_5406b_write16(1,0x141F);
		spi_5406b_write16(0,0x0052);
		spi_5406b_write16(1,0x1A23);
		spi_5406b_write16(0,0x0053);
		spi_5406b_write16(1,0x221A);
		spi_5406b_write16(0,0x0054);
		spi_5406b_write16(1,0x1F14);
		spi_5406b_write16(0,0x0055);
		spi_5406b_write16(1,0x0105);
		spi_5406b_write16(0,0x0056);
		spi_5406b_write16(1,0x1803);
		spi_5406b_write16(0,0x0057);
		spi_5406b_write16(1,0x0518);
		spi_5406b_write16(0,0x0058);
		spi_5406b_write16(1,0x0006);
		spi_5406b_write16(0,0x0059);
		spi_5406b_write16(1,0x0203);
		spi_5406b_write16(0,0x005A);
		spi_5406b_write16(1,0x0E05);
		spi_5406b_write16(0,0x005B);
		spi_5406b_write16(1,0x0E01);
		spi_5406b_write16(0,0x005C);
		spi_5406b_write16(1,0x010D);
		spi_5406b_write16(0,0x005D);
		spi_5406b_write16(1,0x050E);
		spi_5406b_write16(0,0x005E);
		spi_5406b_write16(1,0x0402);
		spi_5406b_write16(0,0x005F);
		spi_5406b_write16(1,0x0600);

		spi_5406b_write16(0,0x0033);
		spi_5406b_write16(1,0x0000);
		spi_5406b_write16(0,0x0011);
		spi_5406b_write16(1,0x0F3F);
		spi_5406b_write16(0,0x0007);
		spi_5406b_write16(1,0x1017);
		break;
	  case 15:
			spi_5406b_master_init();
		break;	
		case 16:
			spi_5406b_read16( 0,0);
		break;
		case 17:
			spi_cameo_lcd_on(1);
			break;
		case 18:
			spi_cameo_lcd_on(0);
			break;
    default:

		break;
	}
	
}
#endif
int spidrv_open(struct inode *inode, struct file *filp)
{
	return 0;
}

int spidrv_release(struct inode *inode, struct file *filp)
{
	return 0;
}
#ifdef CONFIG_CAMEO_DAUGHTERBOARD
unsigned char buf[512];	
void spi_cameo_lcd_action(SPI_LCD* spi_lcd)
{

	unsigned int cmd, sX, eX, sY, eY;
	unsigned char* pData;
	size_t ksize;
	unsigned char RG, GB, sizeLo, sizeHi;
	unsigned long Size_byte, j, i;
	int delay_index;
	cmd = spi_lcd->cmd_type;
	sX = spi_lcd->startX;
	eX = spi_lcd->endX;
	sY = spi_lcd->startY;
	eY = spi_lcd->endY;
	RG = 0;
	GB = 0;	
	

	//printk("spi cameo lcd action - start\n");

	if(cmd == 1)
	{
		ksize = spi_lcd->size;
//		data = kmalloc(ksize*2,GFP_USER);
//		copy_from_user(data,spi_lcd->array,ksize*2);
//		printk("1 ksize=%x\n",ksize);
		
	}
	else
	{
//		printk("2");
		ksize = spi_lcd->size;
		RG = (unsigned char)(spi_lcd->RGB >> 8);
		GB = (unsigned char)spi_lcd->RGB;	
	}
		sizeLo = (ksize & 0xFF);
		sizeHi = (ksize>>8) & 0xFF;
		spi_cameo_write8(7,1);
		spi_cameo_write8(4,sizeHi);
		spi_cameo_write8(5,sizeLo);
#ifndef GPM948A0
		spi_cameo_write8(1,0x36);
		spi_cameo_write8(2,0xA0);
		spi_cameo_write8(1, 0x2A);
		spi_cameo_write8(2, 0);
		spi_cameo_write8(2, sX);
		spi_cameo_write8(2, 0);
		spi_cameo_write8(2, eX);
		spi_cameo_write8(1, 0x2B);
		spi_cameo_write8(2, 0);
		spi_cameo_write8(2, sY);
		spi_cameo_write8(2, 0);
		spi_cameo_write8(2, eY);
		spi_cameo_write8(1, 0x2C);
#else


		spi_cameo_write8(1, 0x0);
		spi_cameo_write8(1, 0x20);
		spi_cameo_write8(2, 0);
		spi_cameo_write8(2, sY);
		spi_cameo_write8(1, 0x0);
		spi_cameo_write8(1, 0x21);
		spi_cameo_write8(2, 0);
		spi_cameo_write8(2, sX);

		spi_cameo_write8(1, 0x0);
		spi_cameo_write8(1, 0x39);
		spi_cameo_write8(2, 0);
		spi_cameo_write8(2, sX);
		spi_cameo_write8(1, 0x0);
		spi_cameo_write8(1, 0x38);
		spi_cameo_write8(2, 0);
		spi_cameo_write8(2, eX);
		spi_cameo_write8(1, 0x0);
		spi_cameo_write8(1, 0x37);
		spi_cameo_write8(2, 0);
		spi_cameo_write8(2, sY);
		spi_cameo_write8(1, 0x0);
		spi_cameo_write8(1, 0x36);
		spi_cameo_write8(2, 0);
		spi_cameo_write8(2, eY);
		spi_cameo_write8(1, 0x0);
		spi_cameo_write8(1, 0x22);
#endif
	if(cmd == 1)
	{	

		i = ksize;
		pData = (unsigned char*)spi_lcd->array;

	   spi_chip_select(ENABLE);
		spi_write(6);
		spi_chip_select(DISABLE);	
		delay_index=CAMEO_DELAY;
		while(delay_index--);
		
		Size_byte = ksize * 2;

		for(j=0; j < Size_byte; j+=512, pData+=512)
		{
			if((Size_byte - j) > 512)
				copy_from_user((unsigned char*)buf,(unsigned char*)(pData),512);
			else
				copy_from_user((unsigned char*)buf,(unsigned char*)(pData),(Size_byte - j));

			for(i=0; i< 512; i++)
			{
				if((i+j) == Size_byte) break;

				spi_chip_select(ENABLE);
				spi_write(buf[i]);
				spi_chip_select(DISABLE);
				delay_index=CAMEO_DELAY;
				while(delay_index--);

			}
		}

//		printk("3 . %x , hi(%x) lo(%x) i:%x\n",ksize,sizeHi,sizeLo,i);
	}
	else
	{
//		printk("4");
		i = ksize;
		
		while(i--){
			spi_cameo_write8(2, RG);	
			spi_cameo_write8(2, GB);
		}

	}
//		printk("spi cameo lcd action - end\n");
}

int spi_LCM_data(unsigned char* lcd_data,int size)
{
	int i,j, delay_index;
	for(i=0,j=0; i< size; i++)
	{

		spi_chip_select(ENABLE);
		spi_write(*(lcd_data+i));
		spi_chip_select(DISABLE);

		delay_index=5000;
		while(delay_index--);
		j++;
	}
	return j;
}
#endif

int spidrv_ioctl(struct inode *inode, struct file *filp,
		unsigned int cmd, unsigned long arg)
{
	unsigned int address, value, size;
	SPI_WRITE *spi_wr;
#if defined (CONFIG_MAC_TO_MAC_MODE) || defined (CONFIG_P5_RGMII_TO_MAC_MODE)
	SPI_VTSS *vtss;
#endif
#ifdef CONFIG_CAMEO_DAUGHTERBOARD

	SPI_LCD *spi_lcd;
   unsigned char* lcd_data;
	unsigned int lcd_cmd;
#endif
	switch (cmd) {
	case RT2880_SPI_READ:
		value = 0; address = 0;

		address = (unsigned int)arg;
		spi_master_init();

		spi_eeprom_read(address, 4, (unsigned char*)&value);
		printk("0x%04x : 0x%08x\n", address, value);
		break;
	case RT2880_SPI_WRITE:
		spi_wr = (SPI_WRITE*)arg;
		address = spi_wr->address;
		value   = spi_wr->value;
		size    = spi_wr->size;

		spi_master_init();
		spi_eeprom_write(address, size, (unsigned char*)&value);
		break;
#ifdef CONFIG_CAMEO_DAUGHTERBOARD
	case RT2880_SPI_LCD_CMD:

		spi_lcd = (SPI_LCD*)arg;		
		spi_cameo_lcd_action(spi_lcd);
	case RT2880_SPI_LCD:
		spi_cameo_lcd((unsigned int )arg);
		break;
	case RT2880_SPI_LCD_DATA:
		lcd_data = kmalloc(512,GFP_ATOMIC);
		copy_from_user(lcd_data, (unsigned char*)arg, 512);
		spi_LCM_data(lcd_data,512);
		kfree(lcd_data);				
		break;
#endif
#if defined (CONFIG_MAC_TO_MAC_MODE) || defined (CONFIG_P5_RGMII_TO_MAC_MODE)
	case RT2880_SPI_INIT_VTSS_NOVLAN:
		vtss_reset();
		vtss_init(0);
		break;
	case RT2880_SPI_INIT_VTSS_VLAN:
		vtss_reset();
		vtss_init(1);
		break;
	case RT2880_SPI_VTSS_READ:
		vtss = (SPI_VTSS *)arg;
		printk("r %x %x %x -> ", vtss->blk, vtss->sub, vtss->reg);
		spi_vtss_read(vtss->blk, vtss->sub, vtss->reg, (u32*)&vtss->value);
		printk("%x\n", (u32)vtss->value);
		break;
	case RT2880_SPI_VTSS_WRITE:
		vtss = (SPI_VTSS *)arg;
		printk("w %x %x %x -> %x\n", vtss->blk, vtss->sub, vtss->reg, (u32)vtss->value);
		spi_vtss_write(vtss->blk, vtss->sub, vtss->reg, (u32)vtss->value);
		break;
#endif
	default:
		printk("spi_drv: command format error\n");
	}

	return 0;
}

struct file_operations spidrv_fops = {
    ioctl:      spidrv_ioctl,
    open:       spidrv_open,
    release:    spidrv_release,
};

static int spidrv_init(void)
{
#ifdef CONFIG_CAMEO_DAUGHTERBOARD
	int delay_index;
#endif
#ifdef  CONFIG_DEVFS_FS
    if(devfs_register_chrdev(spidrv_major, SPI_DEV_NAME , &spidrv_fops)) {
	printk(KERN_WARNING " spidrv: can't create device node\n");
	return -EIO;
    }

    devfs_handle = devfs_register(NULL, SPI_DEV_NAME, DEVFS_FL_DEFAULT, spidrv_major, 0,
				S_IFCHR | S_IRUGO | S_IWUGO, &spidrv_fops, NULL);
#else
    int result=0;
    result = register_chrdev(spidrv_major, SPI_DEV_NAME, &spidrv_fops);
    if (result < 0) {
        printk(KERN_WARNING "spi_drv: can't get major %d\n",spidrv_major);
        return result;
    }

    if (spidrv_major == 0) {
	spidrv_major = result; /* dynamic */
    }
#endif
    //use normal(SPI) mode instead of GPIO mode
#ifdef CONFIG_RALINK_RT2880
    RT2880_REG(RALINK_REG_GPIOMODE) &= ~(1 << 2);
#elif defined (CONFIG_RALINK_RT3052) || defined (CONFIG_RALINK_RT2883)
    RT2880_REG(RALINK_REG_GPIOMODE) &= ~(1 << 1);
#else
#error Ralink Chip not defined
#endif

#ifdef CONFIG_CAMEO_DAUGHTERBOARD
	 
	 
	 spi_cameo_master_init();


	 delay_index=5000;
	 while(delay_index--);

	 spi_cameo_reset_pic();
#endif

    printk("spidrv_major = %d\n", spidrv_major);
    return 0;
}


static void spidrv_exit(void)
{
    printk("spi_drv exit\n");

#ifdef  CONFIG_DEVFS_FS
    devfs_unregister_chrdev(spidrv_major, SPI_DEV_NAME);
    devfs_unregister(devfs_handle);
#else
    unregister_chrdev(spidrv_major, SPI_DEV_NAME);
#endif
}


#if defined (CONFIG_RALINK_PCM) || defined (CONFIG_RALINK_PCM_MODULE)
static unsigned char high8bit (unsigned short data)
{
return ((data>>8)&(0x0FF));
}

static unsigned char low8bit (unsigned short data)
{
return ((unsigned char)(data & 0x00FF));
}
static unsigned short SixteenBit (unsigned char hi, unsigned char lo)
{
	unsigned short data = hi;
	data = (data<<8)|lo;
	return data;
}
void spi_si3220_master_init(void)
{
	int i;
	/* reset spi block */
	RT2880_REG(RT2880_RSTCTRL_REG) |= RSTCTRL_SPI_RESET;
	RT2880_REG(RT2880_RSTCTRL_REG) &= ~(RSTCTRL_SPI_RESET);
	/* udelay(500); */
	for ( i = 0; i < 1000; i++);
	

	RT2880_REG(RT2880_SPICFG_REG) = SPICFG_MSBFIRST | 
									SPICFG_RXCLKEDGE_FALLING |
									SPICFG_TXCLKEDGE_FALLING |
									SPICFG_SPICLK_DIV128 | SPICFG_SPICLKPOL;

	spi_chip_select(DISABLE);
#ifdef DBG
	printk("[slic]SPICFG = %08x\n", RT2880_REG(RT2880_SPICFG_REG));
	printk("[slic]is busy %d\n", IS_BUSY);
#endif		 	
}

u8 spi_si3220_read8(unsigned char cid, unsigned char reg)
{
	u8 value;
	
	spi_si3220_master_init();
	//spi_chip_select(ENABLE);
	spi_write(cid);
	spi_chip_select(DISABLE);
	spi_chip_select(ENABLE);
	spi_write(reg);
	spi_chip_select(DISABLE);
	spi_chip_select(ENABLE);
	value = spi_read();
	spi_chip_select(DISABLE);
	return value;
}

void spi_si3220_write8(unsigned char cid, unsigned char reg, unsigned char value)
{ 
	spi_si3220_master_init();
	//spi_chip_select(ENABLE);
	spi_write(cid);
	spi_chip_select(DISABLE);
	spi_chip_select(ENABLE);
	spi_write(reg);
	spi_chip_select(DISABLE);
	spi_chip_select(ENABLE);
	spi_write(value);
	spi_chip_select(DISABLE);
}

unsigned short spi_si3220_read16(unsigned char cid, unsigned char reg)
{
	unsigned char hi, lo;
	spi_si3220_master_init();
	//spi_chip_select(ENABLE);
	spi_write(cid);
	spi_write(reg);
	spi_chip_select(DISABLE);
	spi_chip_select(ENABLE);
	hi = spi_read();
	lo = spi_read();
	spi_chip_select(DISABLE);
	return SixteenBit(hi,lo);
}

void spi_si3220_write16(unsigned char cid, unsigned char reg, unsigned short value)
{
	unsigned char hi, lo;
	
	hi = high8bit(value);
	lo = low8bit(value);
	spi_si3220_master_init();
	//spi_chip_select(ENABLE);
	spi_write(cid);
	spi_write(reg);
	spi_chip_select(DISABLE);
	spi_chip_select(ENABLE);
	spi_write(hi);
	spi_write(lo);
	spi_chip_select(DISABLE);
}

void spi_si321x_master_init(void)
{
	RT2880_REG(RT2880_SPICFG_REG) =	SPICFG_MSBFIRST | 
									SPICFG_SPICLK_DIV128 | SPICFG_SPICLKPOL |
									SPICFG_TXCLKEDGE_FALLING;
	spi_chip_select(DISABLE);
#ifdef DBG
	printk("[slic]SPICFG = %08x\n", RT2880_REG(RT2880_SPICFG_REG));
	printk("[slic]is busy %d\n", IS_BUSY);
#endif		 	
}

u8 spi_si321x_read8(unsigned char cid, unsigned char reg)
{
	u8 value;
	spi_si321x_master_init();
	spi_write(reg|0x80);
	spi_chip_select(DISABLE);
	value = spi_read();
	spi_chip_select(DISABLE);
	return value;
}

void spi_si321x_write8(unsigned char cid, unsigned char reg, unsigned char value)
{
	spi_si321x_master_init();
	spi_write(reg&0x7F);
	spi_chip_select(DISABLE);
	spi_write(value);
	spi_chip_select(DISABLE);
	return;
}
#endif
#if defined (CONFIG_RALINK_PCM) || defined (CONFIG_RALINK_PCM_MODULE)
EXPORT_SYMBOL(spi_si3220_read8);
EXPORT_SYMBOL(spi_si3220_read16);
EXPORT_SYMBOL(spi_si3220_write8);
EXPORT_SYMBOL(spi_si3220_write16);
EXPORT_SYMBOL(spi_si321x_write8);
EXPORT_SYMBOL(spi_si321x_read8);
EXPORT_SYMBOL(spi_si3220_master_init);
EXPORT_SYMBOL(spi_si321x_master_init);
#endif


#ifdef CONFIG_CAMEO_DAUGHTERBOARD
EXPORT_SYMBOL(spi_cameo_write8);
EXPORT_SYMBOL(spi_cameo_read8);
#endif


module_init(spidrv_init);
module_exit(spidrv_exit);

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,12)
MODULE_PARM (spidrv_major, "i");
#else
module_param (spidrv_major, int, 0);
#endif

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Ralink SoC SPI Driver");
