/******************************************************************************************************
*       Copyright (C) 2007-2011
*       Copyright © 2007 Marvell International Ltd.
*
*       This program is free software; you can redistribute it and/or
*       modify it under the terms of the GNU General Public License
*       as published by the Free Software Foundation; either version 2
*       of the License, or (at your option) any later version.
*
*       This program is distributed in the hope that it will be useful,
*       but WITHOUT ANY WARRANTY; without even the implied warranty of
*       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*       GNU General Public License for more details.
*
*       You should have received a copy of the GNU General Public License
*       along with this program; if not, write to the Free Software
*       Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*
*
*	$Log: vsys#comm#cmodel#hal#drv#api_dhub.h,v $
*	Revision 1.4  2007-10-30 10:58:43-07  chengjun
*	Remove interrpt from HBO to semaphore. Map dHub channel 0 interrupt to semaphore channel 0.
*
*	Revision 1.3  2007-10-24 21:41:17-07  kbhatt
*	add structures to api_dhub.h
*
*	Revision 1.2  2007-10-12 21:36:54-07  oussama
*	adapted the env to use Alpha's Cmodel driver
*
*	Revision 1.1  2007-08-29 20:27:01-07  lsha
*	Initial revision.
*
*
*	DESCRIPTION:
*	OS independent layer for dHub/HBO/SemaHub APIs.
*
****************************************************************************************************/

#ifndef	__PBRIDGE_DHUB_API
#define	__PBRIDGE_DHUB_API			"        DHUB_API >>>    "
/**	DHUB_API
 */

#define	semaphore_hdl			pbridge_semaphore_hdl
#define	semaphore_cfg			pbridge_semaphore_cfg
#define	semaphore_intr_enable		pbridge_semaphore_intr_enable
#define	semaphore_query			pbridge_semaphore_query
#define	semaphore_push			pbridge_semaphore_push
#define	semaphore_pop			pbridge_semaphore_pop
#define	semaphore_chk_empty		pbridge_semaphore_chk_empty
#define	semaphore_chk_full		pbridge_semaphore_chk_full
#define	semaphore_chk_almostEmpty	pbridge_semaphore_chk_almostEmpty
#define	semaphore_chk_almostFull	pbridge_semaphore_chk_almostFull
#define	semaphore_clr_empty		pbridge_semaphore_clr_empty
#define	semaphore_clr_full		pbridge_semaphore_clr_full
#define	semaphore_clr_almostEmpty	pbridge_semaphore_clr_almostEmpty
#define	semaphore_clr_almostFull	pbridge_semaphore_clr_almostFull

#define	hbo_hdl				pbridge_hbo_hdl
#define	hbo_fifoCtl			pbridge_hbo_fifoCtl
#define	hbo_queue_cfg			pbridge_hbo_queue_cfg
#define	hbo_queue_enable		pbridge_hbo_queue_enable
#define	hbo_queue_clear			pbridge_hbo_queue_clear
#define	hbo_queue_busy			pbridge_hbo_queue_busy
#define	hbo_queue_clear_done		pbridge_hbo_queue_clear_done
#define	hbo_queue_read			pbridge_hbo_queue_read
#define	hbo_queue_write			pbridge_hbo_queue_write

#define	dhub_hdl			pbridge_dhub_hdl
#define	dhub_semaphore			pbridge_dhub_semaphore
#define	dhub_hbo			pbridge_dhub_hbo
#define	dhub_channel_cfg		pbridge_dhub_channel_cfg
#define	dhub_channel_enable		pbridge_dhub_channel_enable
#define	dhub_channel_clear		pbridge_dhub_channel_clear
#define	dhub_channel_flush		pbridge_dhub_channel_flush
#define	dhub_channel_busy		pbridge_dhub_channel_busy
#define	dhub_channel_pending		pbridge_dhub_channel_pending
#define	dhub_channel_clear_done		pbridge_dhub_channel_clear_done
#define	dhub_channel_write_cmd		pbridge_dhub_channel_write_cmd
#define	dhub_channel_generate_cmd	pbridge_dhub_channel_generate_cmd
#define	dhub_channel_big_write_cmd	pbridge_dhub_channel_big_write_cmd

#define	dhub2d_hdl			pbridge_dhub2d_hdl
#define	dhub2d_channel_cfg		pbridge_dhub2d_channel_cfg
#define	dhub2d_channel_enable		pbridge_dhub2d_channel_enable
#define	dhub2d_channel_clear		pbridge_dhub2d_channel_clear
#define	dhub2d_channel_busy		pbridge_dhub2d_channel_busy
#define	dhub2d_channel_clear_done	pbridge_dhub2d_channel_clear_done

#include	"pbridge_ctypes.h"

typedef	struct	HDL_semaphore {
UNSG32		ra;			/*!	Base address of $SemaHub !*/
SIGN32		depth[32];		/*!	Array of semaphore (virtual FIFO) depth !*/
} HDL_semaphore;

typedef	struct	HDL_hbo {
UNSG32			mem;		/*!	Base address of HBO SRAM !*/
UNSG32			ra;		/*!	Base address of $HBO !*/
HDL_semaphore	fifoCtl;			/*!	Handle of HBO.FiFoCtl !*/
UNSG32			base[32];	/*!	Array of HBO queue base address !*/
} HDL_hbo;

typedef	struct	HDL_dhub {
UNSG32			ra;		/*!	Base address of $dHub !*/
HDL_semaphore	semaHub;		/*!	Handle of dHub.SemaHub !*/
HDL_hbo			hbo;		/*!	Handle of dHub.HBO !*/
SIGN32			MTUb[16];	/*!	Array of dHub channel MTU size bits !*/
} HDL_dhub;

typedef	struct	HDL_dhub2d {
UNSG32			ra;		/*!	Base address of $dHub2D !*/
HDL_dhub		dhub;		/*!	Handle of dHub2D.dHub !*/
} HDL_dhub2d;


#ifdef	__cplusplus
	extern	"C"
	{
#endif

/**	SECTION - handle of local contexts
 */
extern	UNSG32	sizeof_hdl_semaphore;	/*!	sizeof(HDL_semaphore) !*/
extern	UNSG32	sizeof_hdl_hbo;		/*!	sizeof(HDL_hbo) !*/
extern	UNSG32	sizeof_hdl_dhub;	/*!	sizeof(HDL_dhub) !*/
extern	UNSG32	sizeof_hdl_dhub2d;	/*!	sizeof(HDL_dhub2d) !*/

/**	ENDOFSECTION
 */



/**	SECTION - API definitions for $SemaHub
 */

/********************************************************************************************
*	Function: semaphore_cfg
*	Description: Configurate a semaphore's depth & reset pointers.
*	Return:	UNSG32			Number of (adr,pair) added to cfgQ
*********************************************************************************************/
UNSG32	semaphore_cfg(
		void		*hdl,		/*!	Handle to HDL_semaphore !*/
		SIGN32		id,		/*!	Semaphore ID in $SemaHub !*/
		SIGN32		depth,		/*!	Semaphore (virtual FIFO) depth !*/
		T64b		cfgQ[]		/*!	Pass NULL to directly init SemaHub, or
							Pass non-zero to receive programming sequence
							in (adr,data) pairs
							!*/
		);

/********************************************************************************************
*	Function: semaphore_intr_enable
*	Description: Configurate interrupt enable bits of a semaphore.
*********************************************************************************************/
void	semaphore_intr_enable(
		void		*hdl,		/*!	Handle to HDL_semaphore !*/
		SIGN32		id,		/*!	Semaphore ID in $SemaHub !*/
		SIGN32		empty,		/*!	Interrupt enable for CPU at condition 'empty' !*/
		SIGN32		full,		/*!	Interrupt enable for CPU at condition 'full' !*/
		SIGN32		almostEmpty,	/*!	Interrupt enable for CPU at condition 'almostEmpty' !*/
		SIGN32		almostFull,	/*!	Interrupt enable for CPU at condition 'almostFull' !*/
		SIGN32		cpu		/*!	CPU ID (0/1/2) !*/
		);

/**	ENDOFSECTION
 */



/**	SECTION - API definitions for $HBO
 */

/********************************************************************************************
*	DEFINITION - convert HBO FIFO control to semaphore control
*********************************************************************************************/
#define	hbo_queue_intr_enable		(hdl,id,empty,full,almostEmpty,almostFull,cpu)		\
		semaphore_intr_enable		(hbo_fifoCtl(hdl),id,empty,full,almostEmpty,almostFull,cpu)

#define	hbo_queue_query(hdl,id,master,ptr)						\
		semaphore_query(hbo_fifoCtl(hdl),id,master,ptr)

#define	hbo_queue_push(hdl,id,delta)							\
		semaphore_push(hbo_fifoCtl(hdl),id,delta)

#define	hbo_queue_pop(hdl,id,delta)							\
		semaphore_pop(hbo_fifoCtl(hdl),id,delta)

#define	hbo_queue_chk_empty(hdl,id)							\
		semaphore_chk_empty(hbo_fifoCtl(hdl),id)

#define	hbo_queue_chk_full(hdl,id)							\
		semaphore_chk_full(hbo_fifoCtl(hdl),id)

#define	hbo_queue_chk_almostEmpty(hdl,id)						\
		semaphore_chk_almostEmpty(hbo_fifoCtl(hdl),id)

#define	hbo_queue_chk_almostFull(hdl,id)						\
		semaphore_chk_almostFull(hbo_fifoCtl(hdl),id)

#define	hbo_queue_clr_empty(hdl,id)							\
		semaphore_clr_empty(hbo_fifoCtl(hdl),id)

#define	hbo_queue_clr_full(hdl,id)							\
		semaphore_clr_full(hbo_fifoCtl(hdl),id)

#define	hbo_queue_clr_almostEmpty	(hdl,id)						\
		semaphore_clr_almostEmpty(hbo_fifoCtl(hdl),id)

#define	hbo_queue_clr_almostFull(hdl,id)							\
		semaphore_clr_almostFull(hbo_fifoCtl(hdl),id)

/**	ENDOFSECTION
*/



/**	SECTION - API definitions for $dHubReg
*/
/********************************************************************************************
*	Function: dhub_hdl
*	Description: Initialize HDL_dhub with a $dHub BIU instance.
*********************************************************************************************/
void	dhub_hdl(
		UNSG32		mem,		/*!	Base address of dHub.HBO SRAM !*/
		UNSG32		ra,		/*!	Base address of a BIU instance of $dHub !*/
		void		*hdl		/*!	Handle to HDL_dhub !*/
		);

/********************************************************************************************
*	Function: dhub_semaphore
*	Description: Get HDL_semaphore pointer from a dHub instance.
*	Return:	void*	Handle for dHub.SemaHub
*********************************************************************************************/
void*	dhub_semaphore(
		void		*hdl		/*!	Handle to HDL_dhub !*/
		);

/********************************************************************************************
*	Function: dhub_hbo_fifoCtl
*	Description: Get HDL_semaphore pointer from the HBO of a dHub instance.
*	Return:	void*		Handle for dHub.HBO.FiFoCtl
*********************************************************************************************/
#define	dhub_hbo_fifoCtl(hdl)		(hbo_fifoCtl(dhub_hbo(hdl)))

/********************************************************************************************
*	DEFINITION - convert from dHub channel ID to HBO FIFO ID & semaphore (interrupt) ID
*********************************************************************************************/
// removed connection from HBO interrupt to semaphore
//#define	dhub_id2intr(id)			((id)+1)
#define	dhub_id2intr(id)			((id))
#define	dhub_id2hbo_cmdQ(id)		((id)*2)
#define	dhub_id2hbo_data(id)		((id)*2+1)

/********************************************************************************************
*	DEFINITION - convert dHub cmdQ/dataQ/channel-done interrupt control to HBO/semaphore control
*********************************************************************************************/
//removed connection from HBO interrupt to semaphore
//#define	dhub_hbo_intr_enable		(hdl,id,empty,full,almostEmpty,almostFull,cpu)
//		  semaphore_intr_enable		(dhub_semaphore(hdl),0,empty,full,almostEmpty,almostFull,cpu)

#define	dhub_channel_intr_enable	(hdl,id,full,cpu)							\
		 semaphore_intr_enable	(dhub_semaphore(hdl),dhub_id2intr(id),0,full,0,0,cpu)

#define	dhub_hbo_cmdQ_intr_enable	(hdl,id,empty,almostEmpty,cpu)					\
		  hbo_queue_intr_enable	(dhub_hbo(hdl),dhub_id2hbo_cmdQ(id),empty,0,almostEmpty,0,cpu)

#define	dhub_hbo_data_intr_enable	(hdl,id,empty,full,almostEmpty,almostFull,cpu)				\
		  hbo_queue_intr_enable	(dhub_hbo(hdl),dhub_id2hbo_data(id),empty,full,almostEmpty,almostFull,cpu)

/********************************************************************************************
*	DEFINITION - convert dHub cmdQ opehdltions to HBO FIFO opehdltions
*********************************************************************************************/
#define	dhub_cmdQ_query		(hdl,id,ptr)					\
		  hbo_queue_query	(dhub_hbo(hdl),dhub_id2hbo_cmdQ(id),0,ptr)

#define	dhub_cmdQ_push		(hdl,id,delta)					\
		  hbo_queue_push	(dhub_hbo(hdl),dhub_id2hbo_cmdQ(id),delta)

/********************************************************************************************
*	DEFINITION - convert dHub dataQ opehdltions to HBO FIFO opehdltions
*********************************************************************************************/
#define	dhub_data_query(hdl,id,master,ptr)\
		  hbo_queue_query(dhub_hbo(hdl),dhub_id2hbo_data(id),master,ptr)

#define	dhub_data_push			(hdl,id,delta)					\
		  hbo_queue_push	(dhub_hbo(hdl),dhub_id2hbo_data(id),delta)

#define	dhub_data_pop			(hdl,id,delta)					\
		  hbo_queue_pop	(dhub_hbo(hdl),dhub_id2hbo_data(id),delta)

/********************************************************************************************
*	Function: dhub_channel_cfg
*	Description: Configurate a dHub channel.
*	Return:		UNSG32		Number of (adr,pair) added to cfgQ, or (when cfgQ==NULL)
*					0 if either cmdQ or dataQ in HBO is still busy
*********************************************************************************************/
UNSG32	dhub_channel_cfg(
		void		*hdl,		/*!Handle to HDL_dhub !*/
		SIGN32		id,		/*!Channel ID in $dHubReg !*/
		UNSG32		baseCmd,	/*!Channel FIFO base address (byte address) for cmdQ !*/
		UNSG32		baseData,	/*!Channel FIFO base address (byte address) for dataQ !*/
		SIGN32		depthCmd,	/*!Channel FIFO depth for cmdQ, in 64b word !*/
		SIGN32		depthData,	/*!Channel FIFO depth for dataQ, in 64b word !*/
		SIGN32		mtu,		/*!See 'dHubChannel.CFG.MTU', 0/1/2 for 8/32/128 bytes !*/
		SIGN32		QoS,		/*!See 'dHubChannel.CFG.QoS' !*/
		SIGN32		selfLoop,	/*!See 'dHubChannel.CFG.selfLoop' !*/
		SIGN32		enable,		/*!0 to disable, 1 to enable !*/
		T64b		cfgQ[]		/*!Pass NULL to directly init dHub, or
						Pass non-zero to receive programming sequence
						in (adr,data) pairs
						!*/
		);

/********************************************************************************************
*	Function: dhub_channel_write_cmd
*	Description: Write a 64b command for a dHub channel.
*	Return:		UNSG32		Number of (adr,pair) added to cfgQ if success, or
*					0 if there're not sufficient space in FIFO
*********************************************************************************************/
UNSG32	dhub_channel_write_cmd(
		void		*hdl,		/*!	Handle to HDL_dhub !*/
		SIGN32		id,		/*!	Channel ID in $dHubReg !*/
		UNSG32		addr,		/*!	CMD: buffer address !*/
		SIGN32		size,		/*!	CMD: number of bytes to transfer !*/
		SIGN32		semOnMTU,	/*!	CMD: semaphore operation at CMD/MTU (0/1) !*/
		SIGN32		chkSemId,	/*!	CMD: non-zero to check semaphore !*/
		SIGN32		updSemId,	/*!	CMD: non-zero to update semaphore !*/
		SIGN32		interrupt,	/*!	CMD: raise interrupt at CMD finish !*/
		T64b		cfgQ[],		/*!	Pass NULL to directly update dHub, or
							Pass non-zero to receive programming sequence
							in (adr,data) pairs
							!*/
		UNSG32		*ptr		/*!	Pass in current cmdQ pointer (in 64b word),
							& receive updated new pointer,
							Pass NULL to read from HW
							!*/
		);
/**	ENDOFSECTION
*/

#ifdef	__cplusplus
	}
#endif



/**	__PBRIDGE_DHUB_API
 */
#endif

/**	ENDOFFILE: pbridge_api_dhub.h
 */

