/*++

Module Name:

    queue.c

Abstract:

    This file contains the queue entry points and callbacks.

Environment:

    Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "queue.tmh"
#include "USBuart.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, ElektorUSBuartKMDFQueueInitialize)
#endif

NTSTATUS
ElektorUSBuartKMDFQueueInitialize(
    _In_ WDFDEVICE Device
    )
/*++

Routine Description:


     The I/O dispatch callbacks for the frameworks device object
     are configured in this function.

     A single default I/O Queue is configured for parallel request
     processing, and a driver context memory allocation is created
     to hold our structure QUEUE_CONTEXT.

Arguments:

    Device - Handle to a framework device object.

Return Value:

    VOID

--*/
{
    WDFQUEUE queue;
    NTSTATUS status;
    WDF_IO_QUEUE_CONFIG    queueConfig;

    PAGED_CODE();
    
    //
    // Configure a default queue so that requests that are not
    // configure-fowarded using WdfDeviceConfigureRequestDispatching to goto
    // other queues get dispatched here.
    //
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
         &queueConfig,
        WdfIoQueueDispatchSequential
        );

    queueConfig.EvtIoDeviceControl = ElektorUSBuartKMDFEvtIoDeviceControl;
    queueConfig.EvtIoStop = ElektorUSBuartKMDFEvtIoStop;

    status = WdfIoQueueCreate(
                 Device,
                 &queueConfig,
                 WDF_NO_OBJECT_ATTRIBUTES,
                 &queue
                 );

    if( !NT_SUCCESS(status) ) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "WdfIoQueueCreate failed %!STATUS!", status);
        return status;
    }

    return status;
}

VOID
ElektorUSBuartKMDFEvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
    )
/*++

Routine Description:

    This event is invoked when the framework receives IRP_MJ_DEVICE_CONTROL request.

Arguments:

    Queue -  Handle to the framework queue object that is associated with the
             I/O request.

    Request - Handle to a framework request object.

    OutputBufferLength - Size of the output buffer in bytes

    InputBufferLength - Size of the input buffer in bytes

    IoControlCode - I/O control code.

Return Value:

    VOID

--*/
{
	WDFDEVICE           device;
	PDEVICE_CONTEXT     pDevContext;
	PIN_RECORD          inRecord = NULL;
	POUT_RECORD         outRecord = NULL;
	size_t              bytesReturned = 0;
	NTSTATUS            status = STATUS_INVALID_DEVICE_REQUEST;

	PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, 
                TRACE_QUEUE, 
                "%!FUNC! Queue 0x%p, Request 0x%p OutputBufferLength %d InputBufferLength %d IoControlCode %d", 
                Queue, Request, (int) OutputBufferLength, (int) InputBufferLength, IoControlCode);

	//
	// initialize variables
	//
	device = WdfIoQueueGetDevice(Queue);
	pDevContext = DeviceGetContext(device);

	// Break for unknown control requests
	if (IoControlCode != IOCTL_USBUART_FUNCTION_REQUEST) {
		status = STATUS_INVALID_DEVICE_REQUEST;
		WdfRequestComplete(Request, status);
		return;
	}

	// Retrieve input buffer
	status = WdfRequestRetrieveInputBuffer(Request,
		sizeof(IN_RECORD),
		&inRecord,
		NULL);

	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE,
			"User's input buffer is too small for this IOCTL, expecting an IN_RECORD\n");
		return;
	}

	// Retrieve output buffer
	status = WdfRequestRetrieveOutputBuffer(Request,
		sizeof(OUT_RECORD),
		&outRecord,
		NULL);

	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE,
			"User's output buffer is too small for this IOCTL, expecting an OUT_RECORD\n");
		return;
	}

	switch (inRecord->bFunction) {
	case IOCTL_USBUART_READ_PORT:
		status = ReadWritePort(
			pDevContext, 
			USBUART_READ_PORT_USB_REQUEST, 
			inRecord->bValue1, 
			inRecord->bValue2, 
			&outRecord->bValue1
		);

		bytesReturned = sizeof(OUT_RECORD);
		break;

	case IOCTL_USBUART_WRITE_PORT:
		status = ReadWritePort(
			pDevContext, 
			USBUART_WRITE_PORT_USB_REQUEST, 
			inRecord->bValue1, 
			inRecord->bValue2, 
			&outRecord->bValue1
		);
		
		bytesReturned = sizeof(OUT_RECORD);
		break;

	case IOCTL_USBUART_WRITE_ISINK_AND_PULLUPS:
		status = ReadWritePort(
			pDevContext, 
			USBUART_WRITE_ISINK_AND_PULLUPS_USB_REQUEST, 
			inRecord->bValue1, 
			inRecord->bValue2, 
			&outRecord->bValue1
		);

		bytesReturned = sizeof(OUT_RECORD);
		break;
	}

	if NT_SUCCESS(status)
		outRecord->bAck = 1;
	else
		outRecord->bAck = 0;

	WdfRequestCompleteWithInformation(Request, status, bytesReturned);

    return;
}

VOID
ElektorUSBuartKMDFEvtIoStop(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ ULONG ActionFlags
)
/*++

Routine Description:

    This event is invoked for a power-managed queue before the device leaves the working state (D0).

Arguments:

    Queue -  Handle to the framework queue object that is associated with the
             I/O request.

    Request - Handle to a framework request object.

    ActionFlags - A bitwise OR of one or more WDF_REQUEST_STOP_ACTION_FLAGS-typed flags
                  that identify the reason that the callback function is being called
                  and whether the request is cancelable.

Return Value:

    VOID

--*/
{
    TraceEvents(TRACE_LEVEL_INFORMATION, 
                TRACE_QUEUE, 
                "%!FUNC! Queue 0x%p, Request 0x%p ActionFlags %d", 
                Queue, Request, ActionFlags);

    //
    // In most cases, the EvtIoStop callback function completes, cancels, or postpones
    // further processing of the I/O request.
    //
    // Typically, the driver uses the following rules:
    //
    // - If the driver owns the I/O request, it either postpones further processing
    //   of the request and calls WdfRequestStopAcknowledge, or it calls WdfRequestComplete
    //   with a completion status value of STATUS_SUCCESS or STATUS_CANCELLED.
    //  
    //   The driver must call WdfRequestComplete only once, to either complete or cancel
    //   the request. To ensure that another thread does not call WdfRequestComplete
    //   for the same request, the EvtIoStop callback must synchronize with the driver's
    //   other event callback functions, for instance by using interlocked operations.
    //
    // - If the driver has forwarded the I/O request to an I/O target, it either calls
    //   WdfRequestCancelSentRequest to attempt to cancel the request, or it postpones
    //   further processing of the request and calls WdfRequestStopAcknowledge.
    //
    // A driver might choose to take no action in EvtIoStop for requests that are
    // guaranteed to complete in a small amount of time. For example, the driver might
    // take no action for requests that are completed in one of the driver’s request handlers.
    //

    return;
}

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
ReadWritePort(
	_In_ PDEVICE_CONTEXT DevContext,
	_In_ UCHAR Function,
	_In_ USHORT Port,
	_In_ USHORT Value,
	_Out_ PUCHAR ReturnValue
)
/*++
Routine Description
This routine writes a value to a given I/O port
Arguments:
DevContext - The USB device
Function - Write or read
Port - The port
Value - The set value
Return Value:
NT status value
--*/
{
	NTSTATUS status;
	WDF_USB_CONTROL_SETUP_PACKET    controlSetupPacket;
	WDF_REQUEST_SEND_OPTIONS        sendOptions;
	WDF_MEMORY_DESCRIPTOR memDesc;
	ULONG    bytesTransferred;
	ULONG64 controlResponse = 0;

	//PAGED_CODE();
	TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_QUEUE, "--> ReadWritePort\n");

	WDF_REQUEST_SEND_OPTIONS_INIT(
		&sendOptions,
		WDF_REQUEST_SEND_OPTION_TIMEOUT
	);

	WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(
		&sendOptions,
		DEFAULT_CONTROL_TRANSFER_TIMEOUT
	);

	WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(&controlSetupPacket,
		BmRequestDeviceToHost, // Setup bmRequestType
		BmRequestToEndpoint, // Setup bmRequestType
		Function, // Setup bRequest (Function)
		Port, // Setup wValue (Port)
		Value); // Setup wIndex (Value)

	WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memDesc,
		&controlResponse,
		sizeof(controlResponse));

	status = WdfUsbTargetDeviceSendControlTransferSynchronously(
		DevContext->UsbDevice,
		NULL, // Optional WDFREQUEST
		&sendOptions,
		&controlSetupPacket,
		&memDesc,
		&bytesTransferred);

	*ReturnValue = (UCHAR) (controlResponse >> 8);

	if (!NT_SUCCESS(status)) {

		TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE,
			"ReadWritePort: Failed to ReadWritePort - 0x%x \n", status);

	}
	else {

		TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_QUEUE,
			"ReadWritePort: Response is 0x%llx\n", controlResponse);

	}

	TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_QUEUE, "<-- ReadWritePort\n");

	return status;

}