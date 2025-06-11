#define UART_ID uart1
#define BAUD_RATE 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE

// For the ADCS board, the GPS is GPIO 2 and 3, and are UART0 and function 11
// For the Motor board, the ADCS is connected to GPIO 40 and 41, and are UART1 and function 2

// When changing these, confirm which uart is being used.  Some pins need
// the GPIO_FUNC_UART_AUX function code.
#define UART_TX_PIN 4
#define UART_RX_PIN 5

// Function prototypes
void on_uart_rx(void);  // I think this should not be here
uint8_t init_uart(void);
