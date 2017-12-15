/*++

Module Name:

    queue.h

Abstract:

    This file contains the queue definitions.

Environment:

    Kernel-mode Driver Framework

--*/

EXTERN_C_START

//
// This is the context that can be placed per queue
// and would contain per queue information.
//
typedef struct _QUEUE_CONTEXT {

    ULONG PrivateDeviceData;  // just a placeholder

} QUEUE_CONTEXT, *PQUEUE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(QUEUE_CONTEXT, QueueGetContext)

NTSTATUS
ElektorUSBuartKMDFQueueInitialize(
    _In_ WDFDEVICE Device
    );

//
// Events from the IoQueue object
//
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL ElektorUSBuartKMDFEvtIoDeviceControl;
EVT_WDF_IO_QUEUE_IO_STOP ElektorUSBuartKMDFEvtIoStop;

// Function to read or write register
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
ReadWritePort(
	_In_ PDEVICE_CONTEXT DevContext,
	_In_ UCHAR Function,
	_In_ USHORT Port,
	_In_ USHORT Value,
	_Out_ PUCHAR ReturnValue
);

EXTERN_C_END
