/********************************************************************************
 * Marvell GPL License Option
 *
 * If you received this File from Marvell, you may opt to use, redistribute and/or
 * modify this File in accordance with the terms and conditions of the General
 * Public License Version 2, June 1991 (the "GPL License"), a copy of which is
 * available along with the File in the license.txt file or by writing to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
 * on the worldwide web at http://www.gnu.org/licenses/gpl.txt.
 *
 * THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
 * DISCLAIMED.  The GPL License provides additional details about this warranty
 * disclaimer.
 ******************************************************************************/

#ifndef _BYTE_REV_H_
#define _BYTE_REV_H_

#include "types.h"

inline uint16_t byte_rev16(uint16_t n)
{
	return ((n << 8) & 0xff00) | ((n >> 8) & 0x00ff);
}

inline uint32_t byte_rev32(uint32_t n)
{
	return  ((n << 24) & 0xff000000) |
		((n <<  8) & 0x00ff0000) |
		((n >>  8) & 0x0000ff00) |
		((n >> 24) & 0x000000ff);
}


#endif /* _BYTE_REV_H_ */
