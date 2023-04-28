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

#ifndef _BIT_H_
#define _BIT_H_

/*
 * bit definition
 */

/** Define a bit							*/
#define BIT(k)			(1UL << (k))
#define BIT64(k)		(1ULL << (k))

/** Define a continuous bit field with NUM bits				*/
#define BITS(start, num)	((~((-1UL) << (num))) << (start))
#define BITS64(start, num)	((~((-1ULL) << (num))) << (start))

/*
 * bit operation
 */

/** Clear a bit								*/
#define bit_clear(src, bit)	\
	((src) & (~BIT(bit)))

/** Set a bit								*/
#define bit_set(src, bit)	\
	((src) & BIT(bit))

/** Get a bit								*/
#define bit_get(src, bit)	\
	(((src) >> (bit)) & 0x1)

/** Check a bit whether it's set (0x1)					*/
#define bit_check(src, bit)	\
	((src) & BIT(bit))


/*
 * bits operation
 */

/** Clear a masked area in a value					*/
#define bits_clear(src, mask)	\
	((src) & (~(mask)))

/** Set a masked area to a value					*/
#define bits_set(src, val, mask, shift) \
	((((src) & ~(mask)) | (((val) << (shift)) & (mask))))

/** Get a field in a register						*/
#define bits_get(src, mask, shift)	\
	(((src) & (mask)) >> (shift))

/** Check whether the field is not 0					*/
#define bits_check(src, mask)	\
	((src) & (mask))

#endif /* _BIT_H_ */
