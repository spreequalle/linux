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

#ifndef _BIT_CNT_H_
#define _BIT_CNT_H_

#include "types.h"

/** count 8 bits.
 *
 * @param n                 Data to count.
 *
 * @return unsigned char    The countd data.
 *
 * @sa count_bit16(), count_bit32()
 */
inline uint8_t bit_cnt8(uint8_t n)
{
	n = ((n >> 1) & 0x55) + (n & 0x55);
	n = ((n >> 2) & 0x33) + (n & 0x33);
	n = ((n >> 4) & 0x0f) + (n & 0xf0);

	return n;
}

/** count 16 bits.
 *
 * @param n                 Data to count.
 *
 * @return unsigned short   The countd data.
 *
 * @sa count_bit16(), count_bit32()
 */
inline uint16_t bit_cnt16(uint16_t n)
{
	n = ((n >> 1) & 0x5555) + (n & 0xaaaa);
	n = ((n >> 2) & 0x3333) + (n & 0xcccc);
	n = ((n >> 4) & 0x0f0f) + (n & 0xf0f0);
	n = ((n >> 8) & 0x00ff) + (n & 0xff00);

	return n;
}

/** count 32 bits.
 *
 * @param n                 Data to count.
 *
 * @return unsigned int    The countd data.
 *
 * @sa count_bit16(), count_bit32()
 */
inline uint32_t bit_cnt32(uint32_t n)
{
	n = ((n >> 1) & 0x55555555) + (n & 0xaaaaaaaa);
	n = ((n >> 2) & 0x33333333) + (n & 0xcccccccc);
	n = ((n >> 4) & 0x0f0f0f0f) + (n & 0xf0f0f0f0);
	n = ((n >> 8) & 0x00ff00ff) + (n & 0xff00ff00);
	n = ((n >> 16) & 0x0000ffff) + (n & 0xffff0000);

	return n;
}

#endif /* _BIT_CNT_H_ */
