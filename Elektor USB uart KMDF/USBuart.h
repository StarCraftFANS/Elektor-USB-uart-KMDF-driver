
// Timeout
extern const __declspec(selectany) LONGLONG DEFAULT_CONTROL_TRANSFER_TIMEOUT = 5 * -1 * WDF_TIMEOUT_TO_SEC;


//
// Define I/O ports
//
#define USBUART_PORT_0     0x00
#define USBUART_PORT_1     0x01


//
// Define the vendor commands supported by our device
//
#define USBUART_WRITE_ISINK_AND_PULLUPS_USB_REQUEST 0x03
#define USBUART_READ_PORT_USB_REQUEST               0x04
#define USBUART_WRITE_PORT_USB_REQUEST              0x05

