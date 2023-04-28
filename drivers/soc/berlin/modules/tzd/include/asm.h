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

#ifndef _ASM_H_
#define _ASM_H_

#include <config.h>

/* Some toolchains use other characters (e.g. '`') to mark new line in macro */
#ifndef ASM_NL
#define ASM_NL		;
#endif

#define LENTRY(name) \
	name:

#define ENTRY(name) \
	.globl name ASM_NL \
	LENTRY(name)

#define END(name) \
	.size name, .-name

#define ENDPROC(name) \
	.type name %function ASM_NL \
	END(name)

#define ENDDATA(name) \
	.type name %object

#define FUNC(name) \
	.globl name ASM_NL \
	.type name %function ASM_NL \
	LENTRY(name)

#define LOCAL_FUNC(name) \
	.type name %function ASM_NL \
	LENTRY(name)

#define END_FUNC(name) \
	END(name)

#define DATA(name) \
	.globl name ASM_NL \
	.type name %object ASM_NL \
	LENTRY(name) \

#define LOCAL_DATA(name) \
	.tpe name %object ASM_NL \
	LENTRY(name)

#define END_DATA(name) \
	END(name)

#ifdef CONFIG_SMP
# define ALT_SMP(instr...)	instr
# define ALT_UP(instr...)
#else /* !CONFIG_SMP */
# define ALT_SMP(instr...)
# define ALT_UP(instr...)	instr
#endif /* CONFIG_SMP */

#endif /* _ASM_H_ */
