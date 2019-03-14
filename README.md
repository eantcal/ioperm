Sample code related to the following article (by Antonino Calderone - published on Computer Programming - n. 153 - Gennaio 2006 

# Enabling direct I/O ports access in user space

This article describes the direct I/O access techniques in Linux and Windows user space applications. A Windows kernel driver which uses undocumented internal API is also described.


Intel x86 processors in addition to memory mapped devices support also so called I/O ports mapped devices via a 'privileged' set of machine instructions. Such instructions are checked by system to guarantee processes and kernel space isolation where only (Linux and Windows) Kernel privileged code is allowed to address the I/O directly.

Only two of four privilege levels (rings) of x86 processors are typically used by Linux and Windows: ring 0 for kernel space and ring 3 for user space. While in kernel space any privileged instruction can be executed, in user space this can only be allowed through a specific interface provided by operating system.

Even if user space applications normally don't access directly the I/O a specific class of applications can violate such rule. We are talking about User Mode Drivers (UMD).

An UMD is a simple user space application since it can be built, executed and debugged as any other user space application while a Kernel mode driver (KMD) requires specific tools and a very specific knowledge to be designed, implemented, deployed and debugged.

Even taking into account that an UMD might represent a violation of modern operating systems architecture which splits user space and kernel space domains, an UMD might still represent a pragmatic temporary solution for experimenting low level stuff without dealing with KMD complexity.

Often a better way is combining UMD and KMD for providing a solution where the KMD provides access to I/O and the UMD implements stuff 'too' complex to be implemented easily in the KMD. Sometimes this is not possible because the performances are compromised by the Kernel / User context switch overhead.

And finally, although we cannot replace a KMD anytime we have to deal with interrupts and other bunch of low level stuff, knowing how to access I/O from user space can still be useful in many other cases.

# I/O permission level and I/O permission bit map
x86 processors use an algorithm to validate a port I/O access based on two permission checks ([1]):

Checking the I/O Privilege Level (IOPL) of EFLAGS register
Checking I/O permission bit map (IOPM) of a process task state segment (TSS)
For the memory mapped devices other MMU related specific mechanisms are provided (but they are out of scope, see [2] for more details on this).

EFLAGS register is the 32 bit status register of processor. EFLAGS is stored into TSS when a task is suspended and it is replaced with the one contained in the new executing task's TSS. EFLAGS IOPL field controls the I/O ports address space restricting machine instruction access to such ports. Instruction as IN, INS, OUT, OUTS can be executed if the Current Privilege Level (CPL) of executing process is less than or equal to IOPL. Such instructions (plus STL and CLI instructions), are defined I/O sensitive. A general protection exception will be raised any time a non privileged process try to use them.

Because each process has a specific copy of EFLAGS, different process might have different privileges. User space processes (CPL=3) cannot modify directly the IOPL, while have to ask operating system to do that. Linux for example provides a syscall named iopl. A root process with CAP_SYS_RAWIO capability can modify the IOPL field getting access to the whole I/O address space.

You can find more information about iopl syscall (sys_iopl) and ioperm syscall (which modifies the I/O permission bitmap which will be soon discussed) looking Linux kernel source code.

I/O permission bitmap is part of TSS. Base address and location are both part of TSS.

By modifying the permission bitmap is possible to enable the I/O port access for any process even less privileged or virtual-8086 processes where CPL>=IOPL. The bitmap can cover the entire I/O address space or a subset of it.

Each bit corresponds to an I/O address space byte. For example 1-byte size port at address 0x31 corresponds to bit 1 of byte 7 of the bitmap. A process is allowed to access a specific byte if the related bit on the bitmap is 0.

Linux ioperm syscall can modify the first 0x3FF ports while the only way to get access to the remaining ports is using iopl syscall.
