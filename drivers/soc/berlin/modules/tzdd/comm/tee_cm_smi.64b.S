	.globl cm_call_smi
cm_call_smi:

	sub sp, sp, #96

	stp x19, x20, [sp, #0]
	stp x21, x22, [sp, #16]
	stp x23, x24, [sp, #32]
	stp x25, x26, [sp, #48]
	stp x27, x28, [sp, #64]
	stp x29, x30, [sp, #80]

	/* flag: ipi or smc */
	ldr w0, [x0]
	/* w1/w2/3/4/5/6 - reserved */

	smc #0

	ldp x29, x30, [sp, #80]
	ldp x27, x28, [sp, #64]
	ldp x25, x26, [sp, #48]
	ldp x23, x24, [sp, #32]
	ldp x21, x22, [sp, #16]
	ldp x19, x20, [sp, #0]

	add sp, sp, #96

	ret
