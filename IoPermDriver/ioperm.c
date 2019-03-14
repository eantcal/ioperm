/* 
 * Author A. Calderone (c) - 2005
 * email:antonino.calderone@gmail.com
 * 
 * This program is free software; you can redistribute it and/or
 * modify it without any restriction.
 * It is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 */


/* -------------------------------------------------------------------------- */

#include <ntddk.h>
#include "ioperm_devcntl.h"


/* -------------------------------------------------------------------------- */

#define IO_BITMAP_BITS  65536
#define IO_BITMAP_BYTES (IO_BITMAP_BITS/8)
#define IOPM_BITMAP_SIZE IO_BITMAP_BYTES
#define PRIVATE static


/* -------------------------------------------------------------------------- */

PRIVATE WCHAR DEVICE_NAME[] = L"\\Device\\ioperm";
PRIVATE WCHAR DEVICE_DOS_NAME[] = L"\\DosDevices\\ioperm";
PRIVATE UNICODE_STRING device_dos_name, device_name;


/* -------------------------------------------------------------------------- */

typedef unsigned char* iopm_bitmap_t;
PRIVATE iopm_bitmap_t iopm_bitmap_copy_ptr = 0;

void Ke386IoSetAccessProcess(PEPROCESS, int);
void Ke386SetIoAccessMap(int, iopm_bitmap_t);

NTSTATUS ioperm_devcntrl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );


/* -------------------------------------------------------------------------- */

NTSTATUS ioperm_create(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information=0;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return(STATUS_SUCCESS);
}


/* -------------------------------------------------------------------------- */

NTSTATUS ioperm_close(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information=0;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return(STATUS_SUCCESS);
}


/* -------------------------------------------------------------------------- */

VOID ioperm_unload(IN PDRIVER_OBJECT DriverObject) {
  if (iopm_bitmap_copy_ptr) {
    MmFreeNonCachedMemory(iopm_bitmap_copy_ptr, IOPM_BITMAP_SIZE);
  }

  IoDeleteSymbolicLink (&device_dos_name);
  IoDeleteDevice(DriverObject->DeviceObject);
}


/* -------------------------------------------------------------------------- */

NTSTATUS DriverEntry( IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath ) {
  PDEVICE_OBJECT deviceObject = 0;
  NTSTATUS status = STATUS_SUCCESS;
  
  iopm_bitmap_copy_ptr = MmAllocateNonCachedMemory(IOPM_BITMAP_SIZE);
      
  if (! iopm_bitmap_copy_ptr)  {
    status = STATUS_INSUFFICIENT_RESOURCES;
    goto error_handler;
  }
    
  RtlInitUnicodeString(&device_name, DEVICE_NAME );
  RtlInitUnicodeString(&device_dos_name, DEVICE_DOS_NAME );

  status = IoCreateDevice(DriverObject, 0,
                          &device_name,
                          FILE_DEVICE_UNKNOWN, 0, FALSE, 
                          &deviceObject);

  if (! NT_SUCCESS(status) ) {
    goto error_handler;
  }
      
  if (! NT_SUCCESS(status = IoCreateSymbolicLink (&device_dos_name, &device_name))) {
    goto error_handler;
  }

  DriverObject->MajorFunction[IRP_MJ_CREATE] = ioperm_create;
  DriverObject->MajorFunction[IRP_MJ_CLOSE] = ioperm_close;
  DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ioperm_devcntrl;
  DriverObject->DriverUnload = ioperm_unload;

error_handler:    
  return status;
}


/* -------------------------------------------------------------------------- */

NTSTATUS
ioperm_devcntrl( IN PDEVICE_OBJECT DeviceObject, IN PIRP pIrp ) {
  NTSTATUS status = STATUS_SUCCESS;   
  pIrp->IoStatus.Information = 0;

  switch (IoGetCurrentIrpStackLocation(pIrp)->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_IOPM_ENABLE_IO:
      RtlZeroMemory(iopm_bitmap_copy_ptr, IOPM_BITMAP_SIZE);
      break;

    case IOCTL_IOPM_DISABLE_IO:
      RtlFillMemory(iopm_bitmap_copy_ptr, IOPM_BITMAP_SIZE, -1);
      break;

    default:
      status = STATUS_UNSUCCESSFUL;
      break;
  }

  if ( NT_SUCCESS( status ) ) {
    PEPROCESS current_process = IoGetCurrentProcess();

    if (current_process) {
      Ke386SetIoAccessMap(1, iopm_bitmap_copy_ptr);
      Ke386IoSetAccessProcess(current_process, 1);
    }
  }     

  pIrp->IoStatus.Status = status;
  IoCompleteRequest( pIrp, IO_NO_INCREMENT );

  return status;
}

