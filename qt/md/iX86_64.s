/* iX386_64.s -- assembly support. */

/*
// QuickThreads -- Threads-building toolkit.
// Copyright (c) 1993 by David Keppel
//
// Permission to use, copy, modify and distribute this software and
// its documentation for any purpose and without fee is hereby
// granted, provided that the above copyright notice and this notice
// appear in all copies.  This software is provided as a
// proof-of-concept and for demonstration purposes; there is no
// representation about the suitability of this software for any
// purpose. */

/* 64-bit Intel Architecture Support
// written by Andy Goodrich, Forte Design Systms, Inc.  */

/* NOTE: double-labeled `_name' and `name' for System V compatability.  */
/* NOTE: Mixed C/C++-style comments used. Sorry! */

    .text
    .align 2

    .globl _qt_abort
    .globl qt_abort
    .globl _qt_block
    .globl qt_block
    .globl _qt_blocki
    .globl qt_blocki
    .globl _qt_align
    .globl qt_align

_qt_abort:
qt_abort:
_qt_block:
qt_block:
_qt_blocki:
qt_blocki:
	                 /* 11 (return address.) */
        pushq %rbp       /* 10 (push stack frame top.) */
	movq  %rsp, %rbp /* set new stack frame top. */
	                 /* save registers. */
	subq $8, %rsp    /*  9 (Stack alignment) */
	pushq %r12       /*  8 ... */
	pushq %r13       /*  7 ... */
	pushq %r14       /*  6 ... */
	pushq %r15       /*  5 ... */
	pushq %rbx       /*  4 ... */
	pushq %rcx       /*  3 ... (new stack address) */
	pushq %rdx       /*  2 ... (arg) */
	pushq %rdi       /*  1 ... (address of save function.) */
	pushq %rsi       /*  0 ... (cor) */

/*
according to other document, it seems the parameter is
rdi 1st
rsi 2nd
rdx 3rd
rcx 4th
r8 5th
r9 6th

rdi is first, but here it's second, it's strange
*/

    movq %rdi, %rax  /* get address of save function. */
    movq %rsp, %rdi  /* set current stack as save argument. */
/*
after this, the old thread's %rsp is saved to %rdi, to act as the first
parameter of function calling in later's "call *%rax".

the function called is the first parameter of the qt_block() function. The
function alled will keep the second and third parameter the same with the
qt_block() function, except the first parameter is changed to the old
thread's %rsp
*/
	movq %rcx, %rsp  /* swap stacks. */
	movq %rcx, %rbp  /* adjust stack frame pointer. */
/*
%rcx is the 4th parameter of qt_block, it's also the stack of the new threads.
by those two instructions, the call stack has changed to the new thread's stack.
*/
	addq $10*8, %rbp /* ... */
    call *%rax      /* call function to save stack pointer. */

/*
after this function call, it would return to the next instruction, with
new thread's stack.

new thread's stack has set it's stack layout, to let cpu execute the
new thread's saved $rid. If the new thread is first time to execute,
it would execute the function pointer.
*/

	                /* restore registers. */
	popq %rsi       /* ... */
	popq %rdi       /* ... */
	popq %rdx       /* ... */
	popq %rcx       /* ... */
	popq %rbx       /* ... */
	popq %r15       /* restore registers from new stack. */
	popq %r14       /* ... */
	popq %r13       /* ... */
	popq %r12       /* ... */
	leave           /* unwind stack. */
/*
a leave is equal to:
movq %rbp, %rsp
popq %rbp

it pop the value pointing by the %rbp, to %rbp, to restore previous
call-frame's %rbp.

When the new thread is initialized, the restored %rbp would be 0. Since
it would not return, a %rbp with 0 would be of problem.
*/
_qt_align:
qt_align:
	ret             /* return. */
/*
the ret instruction may be executed for several times.

a ret is like:
popq %rsp, %rip; pop out [%rsp] to %rip, and sub %rsp with 8

it pop the current stack top to the %rip, which should be the returned address.
but for X86_64, it would pop a value with address of "qt_align", so it would
execute the "ret" instruction again(qt_align value is just the "ret" instruction
address), with the %rsp -= 8(64-bit).

beforing return to the real coroutine function, it would execute an extra
"ret". BY such a design, the coroutine function could be started with
a %rbp aligned with 16byte. Without this extra "qt_align" related ret,
the %rbp would be aligned with 8byte instead. That's why it's called "qt_align".
*/
