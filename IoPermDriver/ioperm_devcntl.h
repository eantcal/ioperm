/* 
 * Author A. Calderone (c) - 2005
 * @email: antonino.calderone@gmail.com
 * 
 * This file is part of ioperm. ioperm is free software; 
 * you can redistribute it and/or modify it without any restriction.
 * It is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 */

#ifndef __IOPERM_DEVCNTL_H__
#define __IOPERM_DEVCNTL_H__

#define IOPERM_DRIVER_IOCTL_CODE    0x8000 
#define IOPERM_DRIVER_IOCTL_ENABLE  0x800
#define IOPERM_DRIVER_IOCTL_DISABLE 0x801

#define IOCTL_IOPM_ENABLE_IO \
    CTL_CODE(IOPERM_DRIVER_IOCTL_CODE, \
             IOPERM_DRIVER_IOCTL_ENABLE, \
             METHOD_BUFFERED, \
             FILE_ANY_ACCESS)

#define IOCTL_IOPM_DISABLE_IO \
    CTL_CODE(IOPERM_DRIVER_IOCTL_CODE, \
             IOPERM_DRIVER_IOCTL_DISABLE, \
             METHOD_BUFFERED, \
             FILE_ANY_ACCESS)
#endif
