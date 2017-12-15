/*++

Module Name:

    public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    user and kernel

--*/

//
// Define an Interface Guid so that app can find the device and talk to it.
//

DEFINE_GUID (GUID_DEVINTERFACE_ElektorUSBuartKMDF,
    0xe111fb47,0x4601,0x4d0f,0xa7,0x54,0xde,0x1d,0x43,0x49,0x4d,0xab);
// {e111fb47-4601-4d0f-a754-de1d43494dab}


//
// Define I/O requests supported by our driver
//

#define IOCTL_USBUART_FUNCTION_REQUEST              0x04
#define IOCTL_USBUART_READ_PORT                  20
#define IOCTL_USBUART_WRITE_PORT                 21
#define IOCTL_USBUART_WRITE_ISINK_AND_PULLUPS    23


//
// Define the structures that will be used by the IOCTL 
//  interface to the driver
//

typedef struct _IN_RECORD {

	UCHAR bFunction;
	UCHAR bValue1;
	UCHAR bValue2;
	UCHAR bValue3;

}IN_RECORD, *PIN_RECORD;

typedef struct _OUT_RECORD {

	UCHAR bAck;
	UCHAR bValue1;
	UCHAR bValue2;
	UCHAR bValue3;

}OUT_RECORD, *POUT_RECORD;
