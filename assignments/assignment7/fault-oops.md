# Desription

The stacktrace shows that tha fault occurs by the execution of the instruction at PC = faulty_write+0x8/0x10 [faulty].

The Objdump shows that the previous instruction is  the following
  8:	b900003f 	str	wzr, [x1]
Which is a store to the address stored in X1, which in turn i 0.



# OOPS Stack Trace

[   30.009913] Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000
[   30.010973] Mem abort info:
[   30.011177]   ESR = 0x0000000096000044
[   30.011446]   EC = 0x25: DABT (current EL), IL = 32 bits
[   30.011777]   SET = 0, FnV = 0
[   30.012014]   EA = 0, S1PTW = 0
[   30.012261]   FSC = 0x04: level 0 translation fault
[   30.012613] Data abort info:
[   30.012840]   ISV = 0, ISS = 0x00000044, ISS2 = 0x00000000
[   30.013191]   CM = 0, WnR = 1, TnD = 0, TagAccess = 0
[   30.013498]   GCS = 0, Overlay = 0, DirtyBit = 0, Xs = 0
[   30.013876] user pgtable: 4k pages, 48-bit VAs, pgdp=0000000043785000
[   30.016589] [0000000000000000] pgd=0000000000000000, p4d=0000000000000000
[   30.017126] Internal error: Oops: 0000000096000044 [#1] PREEMPT SMP
[   30.017633] Modules linked in: hello(O) faulty(O) scull(O) ipv6
[   30.018581] CPU: 0 PID: 169 Comm: sh Tainted: G           O       6.6.18 #1
[   30.019021] Hardware name: linux,dummy-virt (DT)
[   30.019394] pstate: 80000005 (Nzcv daif -PAN -UAO -TCO -DIT -SSBS BTYPE=--)
[   30.019668] pc : faulty_write+0x8/0x10 [faulty]
[   30.020404] lr : vfs_write+0xc4/0x300
[   30.020647] sp : ffff80008027bd20
[   30.020859] x29: ffff80008027bd20 x28: ffff706fc37349c0 x27: 0000000000000000
[   30.021293] x26: 0000000000000000 x25: 0000000000000000 x24: 0000000000000000
[   30.021552] x23: 0000000000000000 x22: ffff80008027bdf0 x21: 0000aaaacee25840
[   30.021929] x20: ffff706fc374e600 x19: 000000000000000c x18: 0000000000000000
[   30.022248] x17: 0000000000000000 x16: 0000000000000000 x15: 0000000000000000
[   30.022756] x14: 0000000000000000 x13: 0000000000000000 x12: 0000000000000000
[   30.023335] x11: 0000000000000000 x10: 0000000000000000 x9 : 0000000000000000
[   30.023944] x8 : 0000000000000000 x7 : 0000000000000000 x6 : 0000000000000000
[   30.024475] x5 : 0000000000000000 x4 : ffffb4ab0f0ae000 x3 : ffff80008027bdf0
[   30.024988] x2 : 000000000000000c x1 : 0000000000000000 x0 : 0000000000000000
[   30.025671] Call trace:
[   30.026082]  faulty_write+0x8/0x10 [faulty]
[   30.026519]  ksys_write+0x70/0x104
[   30.026774]  __arm64_sys_write+0x1c/0x28
[   30.027008]  invoke_syscall+0x48/0x114
[   30.027241]  el0_svc_common.constprop.0+0x40/0xe0
[   30.027494]  do_el0_svc+0x1c/0x28
[   30.027694]  el0_svc+0x40/0xe8
[   30.027867]  el0t_64_sync_handler+0x100/0x12c
[   30.028062]  el0t_64_sync+0x190/0x194
[   30.028653] Code: ???????? ???????? d2800001 d2800000 (b900003f) 
[   30.029155] ---[ end trace 0000000000000000 ]---


# OBJDUMP

./buildroot/output/build/ldd-custom/misc-modules/faulty.ko:     file format elf64-littleaarch64


Disassembly of section .text:

0000000000000000 <faulty_write>:

ssize_t faulty_write (struct file *filp, const char __user *buf, size_t count,
		loff_t *pos)
{
	/* make a simple fault by dereferencing a NULL pointer */
	*(int *)0 = 0;
   0:	d2800001 	mov	x1, #0x0                   	// #0
	return 0;
}
   4:	d2800000 	mov	x0, #0x0                   	// #0
	*(int *)0 = 0;
   8:	b900003f 	str	wzr, [x1]
}
   c:	d65f03c0 	ret

0000000000000010 <faulty_init>:
	.owner = THIS_MODULE
};


int faulty_init(void)
{
  10:	d503233f 	paciasp
  14:	a9be7bfd 	stp	x29, x30, [sp, #-32]!
extern void chrdev_show(struct seq_file *,off_t);

static inline int register_chrdev(unsigned int major, const char *name,
				  const struct file_operations *fops)
{
	return __register_chrdev(major, 0, 256, name, fops);
  18:	90000004 	adrp	x4, 0 <faulty_write>
  1c:	910003fd 	mov	x29, sp
  20:	f9000bf3 	str	x19, [sp, #16]
	int result;

	/*
	 * Register your major, and accept a dynamic number
	 */
	result = register_chrdev(faulty_major, "faulty", &faulty_fops);
  24:	90000013 	adrp	x19, 0 <faulty_write>
  28:	b9400260 	ldr	w0, [x19]
  2c:	90000003 	adrp	x3, 0 <faulty_write>
  30:	91000084 	add	x4, x4, #0x0
  34:	91000063 	add	x3, x3, #0x0
  38:	52802002 	mov	w2, #0x100                 	// #256
  3c:	52800001 	mov	w1, #0x0                   	// #0
  40:	94000000 	bl	0 <__register_chrdev>
	if (result < 0)
  44:	37f800a0 	tbnz	w0, #31, 58 <faulty_init+0x48>
		return result;
	if (faulty_major == 0)
  48:	b9400261 	ldr	w1, [x19]
  4c:	35000041 	cbnz	w1, 54 <faulty_init+0x44>
		faulty_major = result; /* dynamic */
  50:	b9000260 	str	w0, [x19]

	return 0;
  54:	52800000 	mov	w0, #0x0                   	// #0
}
  58:	f9400bf3 	ldr	x19, [sp, #16]
  5c:	a8c27bfd 	ldp	x29, x30, [sp], #32
  60:	d50323bf 	autiasp
  64:	d65f03c0 	ret

0000000000000068 <cleanup_module>:

void faulty_cleanup(void)
{
  68:	d503233f 	paciasp
	unregister_chrdev(faulty_major, "faulty");
  6c:	90000000 	adrp	x0, 0 <faulty_write>
{
  70:	a9bf7bfd 	stp	x29, x30, [sp, #-16]!
}

static inline void unregister_chrdev(unsigned int major, const char *name)
{
	__unregister_chrdev(major, 0, 256, name);
  74:	52802002 	mov	w2, #0x100                 	// #256
  78:	52800001 	mov	w1, #0x0                   	// #0
  7c:	910003fd 	mov	x29, sp
  80:	b9400000 	ldr	w0, [x0]
  84:	90000003 	adrp	x3, 0 <faulty_write>
  88:	91000063 	add	x3, x3, #0x0
  8c:	94000000 	bl	0 <__unregister_chrdev>
}
  90:	a8c17bfd 	ldp	x29, x30, [sp], #16
  94:	d50323bf 	autiasp
  98:	d65f03c0 	ret

000000000000009c <faulty_read>:
{
  9c:	d503233f 	paciasp
  a0:	a9bd7bfd 	stp	x29, x30, [sp, #-48]!
  a4:	d5384100 	mrs	x0, sp_el0
  a8:	910003fd 	mov	x29, sp
  ac:	a90153f3 	stp	x19, x20, [sp, #16]
  b0:	aa0103f4 	mov	x20, x1
  b4:	aa0203f3 	mov	x19, x2
  b8:	f9423c01 	ldr	x1, [x0, #1144]
  bc:	f90017e1 	str	x1, [sp, #40]
  c0:	d2800001 	mov	x1, #0x0                   	// #0
		*(stack_buf + i) = 0xff;
  c4:	d2800282 	mov	x2, #0x14                  	// #20
  c8:	52801fe1 	mov	w1, #0xff                  	// #255
  cc:	910093e0 	add	x0, sp, #0x24
  d0:	94000000 	bl	0 <memset>
 */
static __always_inline struct task_struct *get_current(void)
{
	unsigned long sp_el0;

	asm ("mrs %0, sp_el0" : "=r" (sp_el0));
  d4:	d5384101 	mrs	x1, sp_el0
	 * Asynchronous I/O running in a kernel thread does not have the
	 * TIF_TAGGED_ADDR flag of the process owning the mm, so always untag
	 * the user address before checking.
	 */
	if (IS_ENABLED(CONFIG_ARM64_TAGGED_ADDR_ABI) &&
	    (current->flags & PF_KTHREAD || test_thread_flag(TIF_TAGGED_ADDR)))
  d8:	b9402c22 	ldr	w2, [x1, #44]
	if (count > 4)
  dc:	f100127f 	cmp	x19, #0x4
  e0:	d2800083 	mov	x3, #0x4                   	// #4
  e4:	9a839273 	csel	x19, x19, x3, ls	// ls = plast
	if (IS_ENABLED(CONFIG_ARM64_TAGGED_ADDR_ABI) &&
  e8:	36a80342 	tbz	w2, #21, 150 <faulty_read+0xb4>
 * @index: 0 based bit index (0<=index<64) to sign bit
 */
static __always_inline __s64 sign_extend64(__u64 value, int index)
{
	__u8 shift = 63 - index;
	return (__s64)(value << shift) >> shift;
  ec:	9340de80 	sbfx	x0, x20, #0, #56
		addr = untagged_addr(addr);
  f0:	8a000280 	and	x0, x20, x0

	if (IS_ENABLED(CONFIG_ALTERNATE_USER_ADDRESS_SPACE) ||
	    !IS_ENABLED(CONFIG_MMU))
		return true;

	return (size <= limit) && (addr <= (limit - size));
  f4:	d2e00021 	mov	x1, #0x1000000000000       	// #281474976710656
  f8:	cb130021 	sub	x1, x1, x19
  fc:	eb01001f 	cmp	x0, x1
 100:	aa1303e0 	mov	x0, x19
 104:	540001c9 	b.ls	13c <faulty_read+0xa0>  // b.plast
		return count;
 108:	7100001f 	cmp	w0, #0x0
}
 10c:	d5384101 	mrs	x1, sp_el0
		return count;
 110:	93407c00 	sxtw	x0, w0
 114:	9a931000 	csel	x0, x0, x19, ne	// ne = any
}
 118:	f94017e3 	ldr	x3, [sp, #40]
 11c:	f9423c22 	ldr	x2, [x1, #1144]
 120:	eb020063 	subs	x3, x3, x2
 124:	d2800002 	mov	x2, #0x0                   	// #0
 128:	540001e1 	b.ne	164 <faulty_read+0xc8>  // b.any
 12c:	a94153f3 	ldp	x19, x20, [sp, #16]
 130:	a8c37bfd 	ldp	x29, x30, [sp], #48
 134:	d50323bf 	autiasp
 138:	d65f03c0 	ret
#define uaccess_mask_ptr(ptr) (__typeof__(ptr))__uaccess_mask_ptr(ptr)
static inline void __user *__uaccess_mask_ptr(const void __user *ptr)
{
	void __user *safe_ptr;

	asm volatile(
 13c:	9248fa80 	and	x0, x20, #0xff7fffffffffffff
	might_fault();
	if (should_fail_usercopy())
		return n;
	if (access_ok(to, n)) {
		instrument_copy_to_user(to, from, n);
		n = raw_copy_to_user(to, from, n);
 140:	910093e1 	add	x1, sp, #0x24
 144:	aa1303e2 	mov	x2, x19
 148:	94000000 	bl	0 <__arch_copy_to_user>
 14c:	17ffffef 	b	108 <faulty_read+0x6c>
	/*
	 * Unlike the bitops with the '__' prefix above, this one *is* atomic,
	 * so `volatile` must always stay here with no cast-aways. See
	 * `Documentation/atomic_bitops.txt` for the details.
	 */
	return 1UL & (addr[BIT_WORD(nr)] >> (nr & (BITS_PER_LONG-1)));
 150:	f9400021 	ldr	x1, [x1]
 154:	aa1403e0 	mov	x0, x20
	if (IS_ENABLED(CONFIG_ARM64_TAGGED_ADDR_ABI) &&
 158:	7206003f 	tst	w1, #0x4000000
 15c:	54fffcc0 	b.eq	f4 <faulty_read+0x58>  // b.none
 160:	17ffffe3 	b	ec <faulty_read+0x50>
 164:	94000000 	bl	0 <__stack_chk_fail>

Disassembly of section .plt:

0000000000000000 <.plt>:
	...

Disassembly of section .text.ftrace_trampoline:

0000000000000000 <.text.ftrace_trampoline>:

