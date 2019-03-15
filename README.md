Sample code related to the following article published on Computer Programming - n. 153 - Gennaio 2006 

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

![tss](https://7bcac53c-a-62cb3a1a-s-sites.googlegroups.com/site/eantcal/home/articles-and-publications/enabling-direct-i-o-ports-access-in-user-space/TSS.png)

# Ke386IoSetAccessProcess and Ke386SetIoAccessMap
Windows does not support syscalls like ioperm or iopl but we can try to implement a KMD which will provide similar features as shown in the source code at List-1 that will discuss better in the next paragraph. 

Although such driver is quite simple it is peculiar because of two undocumented Kernel API Ke386IoSetAccessProcess e Ke386SetIoAccessMap. Non official documentation about such APIs is the following:

- void Ke386IoSetAccessProcess (PEPROCESS, int);
This function ask the Kernel to enable access for the current process to the IOPM bitmap. Second argument enables (1) or disables (0) such access.

- void Ke386SetIoAccessMap (int, IOPM *);
Replaces the current process IOPM bitmap (first parameter must be 1). 

- void Ke386QueryIoAccessMap (int, IOPM *);
Returns the current process IOPM. First argument must be 1.

Described functions can be combined to update the I/O permission bitmap of calling process, allowing it to access to the whole I/O address space, as showing in the following fragment of code:

```
char * pIOPM = NULL;
//...
pIOPM = MmAllocateNonCachedMemory(65536 / 8);
RtlZeroMemory(pIOPM, 65536 / 8);
Ke386IoSetAccessProcess(IoGetCurrentProcess(), 1);
```

The Kernel Mode Driver
KMD role is to provide the O/S with the access to a specific device. 

From a user space process point of view a KMD can be handled as special file and handled by syscalls like CreateFile, ReadFile or DeviceIoControl.

From an implementation point of view it is a set of functions registered into and called by I/O Manager during the I/O operations on the controlled device.

# A generic Windows KMD implementation includes:

DriverEntry function called by I/O Manager as soon as the driver is loaded.
Dispatch entry points: functions called on I/O requests which process the I/O Request Packet (IRPs).
Interrupt Service Routines (ISRs): which handle the device Interrupt requests (IRQs)  
Deferred Procedure Calls (DPCs): special routines typically called from ISR to complete a service routine task out of ISR execution context.
Driver implementation shown in the List-1 does not use any ISR or DPC, while it just implements I/O control command used to get access the I/O permission bitmap.

Our driver in fact exports just two specific features: enabling and disabling the direct I/O ports access, implemented via a IOCTL request. Same result can be obtained in Linux calling the iopl syscall (ioperm can be used just for the first 0x3ff ports for historical reasons). 

Driver entry point is implemented the is the following function:

```
Ke386SetIoAccessMap(1, pIOPM);
```

Such function accepts a DRIVER_OBJECT structure pointer which is a unique object is built by Windows upon the driver activation. RegistryPath parameter is a unicode string which represent the registry path name used for configuring the driver.

The returned value is processed by I/O Manager. In case it should be different than STATUS_SUCCESS, the driver will be terminated and removed from memory. 

This function creates a device object and the related name and then registers the dispatch entry points: in our driver they are implemented in the function ioperm_create, ioperm_close and ioperm_devcntrl.

Such functions process the IRPs built by I/O Manager as result of a I/O system request. 

To build the driver binary we have used Microsoft Driver Development Kit (DDK).

DDK provides a tools and libraries to create the driver binary which typically has .sys file extension which is installed in a specific system directory (system32\drivers)

Installation process includes the Windows Register registration which can be done via using a specific .REG as shown in the following example:

Windows Registry Editor Version 5.00


```
[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\ioperm]
"Type"=dword:00000001
"ErrorControl"=dword:00000001
"Start"=dword:00000002
"DisplayName"="ioperm"
"ImagePath"="System32\\DRIVERS\\ioperm.sys"
```

Once installed the driver configuration will be visible via the Device Manager (as shown in Fig.2). 

We have implemented a simple user space program for Windows and Linux (List-2) which probes the parallel port. Such device is typically mapped at port addresses 0x278, 0x378, 0x3BC. Data register (offset 0) is a 8 bit data latch. 

Knowing that a program can detect the device writing and reading back a byte using a specific pattern (skipping pull-down or pull-up values like 0 or 0xFF).

The program can be compiled in Visual Studio or using GNU C++.

![Windows Device Manager](https://sites.google.com/site/eantcal/home/articles-and-publications/enabling-direct-i-o-ports-access-in-user-space/figura2.jpg)

# Conclusion
Looking back the Linux versions we discovered that ioperm and iopl syscalls have been added since early versions.

We are not surprised of such thing, and we are not surprised that Microsoft has decided not to do that.

# References
[1] Intel - "Intel Architecture Software Developer’s Manual - Volume 1:Basic Architecture", Intel, 1999
[2] Intel - "Intel Architecture Software Developer’s Manual, Volume 3" Intel, 1999
[3] P.G. Viscarola, W.A. Mason - "Windows NT - Device Driver Development", MTP, 1999
[5] http://www.ddj.com/articles/1996/9605/
[6] http://www.beyondlogic.org/porttalk/porttalk.htm
[7] http://www.microsoft.com/whdc/devtools/ddk/default.mspx
[8] http://lxr.linux.no/linux-bk+v2.6.11.5/arch/i386/kernel/ioport.c
