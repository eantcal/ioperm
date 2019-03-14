/* 
 * Author A. Calderone (c) - 2005
 * antonino.calderone@gmail.com
 * 
 * This program is free software; you can redistribute it and/or
 * modify it without any restriction.
 * It is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 */


/* -------------------------------------------------------------------------- */

#if defined (_MSC_VER)
#define PER_WIN32
# include <windows.h>
# include <winioctl.h>
# include "ioperm_devcntl.h"
# include <conio.h>
# include <stdio.h>
#elif defined (__GNUC__) 
# include <sys/io.h>
# define PER_LINUX
# include <stdio.h>
# include <stdlib.h>
#define _inp inb_p 
#define _outp(_PORT_,_VALUE_) outb_p(_VALUE_,_PORT_)
#else
# error Please use GNU C++ for Linux or MS Visual C++ compiler for Windows
#endif


/* -------------------------------------------------------------------------- */

// Usual parallel port addresses
unsigned short port_addr[] = {0x378, 0x278, 0x3BC};


/* -------------------------------------------------------------------------- */

#define MAGIC_NUMBER 0xAC


/* -------------------------------------------------------------------------- */

int main() 
{
#ifdef PER_WIN32
  DWORD dwBytesReturned = 0;
  #define	MAX_ERR_STR 256
  char szError[MAX_ERR_STR] = {0};

  //CreateFile opens the device driver and returns a handle that will be used 
  //to access the device driver
  HANDLE h = CreateFile("\\\\.\\ioperm",                      // file name
                        GENERIC_READ | GENERIC_WRITE,         // access mode
                        FILE_SHARE_READ | FILE_SHARE_WRITE,   // share mode
                        0,                                    // Security Attributes
                        OPEN_EXISTING,                        // how to open
                        0,                                    // file attributes: normal
                        0);                                   // handle to template file: NONE

        
  //Check for open errors
  if (h == INVALID_HANDLE_VALUE) {
    sprintf(szError, "Error %ld", GetLastError());
    MessageBox(0, "IoPerm Error", szError, MB_OK|MB_ICONERROR);
    return -1;
  }

  // The DeviceIoControl function sends the IOCTL_IOPM_ENABLE_IO
  // control code directly to a ioperm device driver, 
  // causing the device to perform the ioperm operation enabling or
  // disabling direct I/O access for calling process
  if (! DeviceIoControl(h,                    // handle to device
                        IOCTL_IOPM_ENABLE_IO, // operation
                        NULL,                 // no input data buffer
                        0,                    // size of input data buffer is 0
                        NULL,                 // no output data buffer
                        0,                    // size of output data buffer is 0
                        &dwBytesReturned,     // byte count
                        NULL))                // overlapped information
  {
    sprintf(szError, "Error %ld", GetLastError());
    MessageBox(0, "IoPerm Error", szError, MB_OK|MB_ICONERROR);
    CloseHandle(h);
    return -1;
  }

  //We need to re-schedule 
  Sleep(1);

#elif defined(PER_LINUX)
  if (iopl(3)) {
    perror("iopl");
    exit(-1);
  }
#endif
  

  // Ok, device opened 
  for (unsigned int i=0; 
       i<sizeof(port_addr)/sizeof(port_addr[0]);
       ++i) 
  {
    _outp(port_addr[i], MAGIC_NUMBER);
    
    if ( MAGIC_NUMBER == _inp(port_addr[i]) ) {
      printf("Parallel port found at address 0x%X ", port_addr[i]);  
      printf("(Status Info 0x%02X - Control Info 0x%02X)\n", 
	     _inp(port_addr[i] + 1), 
	     _inp(port_addr[i] + 2));
    } 
  }

#ifdef PER_WIN32
  CloseHandle(h);
#endif

  return 0;
}

