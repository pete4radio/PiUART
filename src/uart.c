#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "uart.h"
#include "main_gps_uart_shared_buffer.h"

static uint8_t already_initialized = 0; // Flag to check if UART is already initialized

// RX interrupt handler
void on_uart_rx() {
    extern int chars_rxed;
    extern char buffer_UART[BUFLEN];
    extern int lfcr_rxed;
    //  storing has three states:  
    //  0 = not storing because no $ yet, 1 = storing, 2 = waiting for  program to ACK
    extern int storing; // Add this as a global or static variable, initialized to 0

    // This function is called when the UART receives data
    while (uart_is_readable(UART_ID)) {
        uint8_t ch = uart_getc(UART_ID);
        buffer_DEBUG[write_here++] = ch; // Store the character in the debug buffer
        if (write_here >= BUFLEN * 10) {
            write_here = 0; // Reset the debug buffer index if it exceeds the size
        }

        if (ch == '\0') {
            continue; // Ignore null characters
        }

        // Start storing only when '$' is seen
        if (storing == 1) {
            if (ch == '$') {
                storing = 1;
                chars_rxed = 0;
                buffer_UART[chars_rxed++] = '$';
                lfcr_rxed = 0; // Reset LF/CR counter
            }
            continue;
        }

        // If buffer is full, null-terminate and stop storing
        if ((chars_rxed > BUFLEN - 2) && (storing == 1)) {
            buffer_UART[BUFLEN - 1] = '\0';
            storing = 0;   // This discards overruns
            return;
        }

        // If newline, terminate string, stop storing, and exit handler
        if (ch == '\n' || ch == '\r') {
            buffer_UART[chars_rxed] = '\0';
            storing = 2; // Set storing to 2 to indicate waiting for ACK
            lfcr_rxed++;
            return;
        }

        // Otherwise, store character and increment counter
        buffer_UART[chars_rxed++] = ch;
    }
}

uint8_t init_uart() {
    // Initialize the UART, and set up the pins
    if (already_initialized) {
        return PICO_OK; // UART already initialized, no need to reinitialize
    }
    already_initialized = 1; // Set the flag to indicate UART is initialized
    
    // Set up our UART with a basic baud rate.
    uart_init(UART_ID, BAUD_RATE);

    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, UART_FUNCSEL_NUM(UART_ID, UART_TX_PIN));
    gpio_set_function(UART_RX_PIN, UART_FUNCSEL_NUM(UART_ID, UART_RX_PIN));

    // Actually, we want a different speed
    // The call will return the actual baud rate selected, which will be as close as
    // possible to that requested
    int __unused actual = uart_set_baudrate(UART_ID, BAUD_RATE);

    // Set UART flow control CTS/RTS, we don't want these, so turn them off
    uart_set_hw_flow(UART_ID, false, false);

    // Set our data format
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);

    // Set the FIFO to be 16 bytes deep, and clear it
    uart_set_fifo_enabled(UART_ID, true);

    // Set up a RX interrupt
    // We need to set up the handler first
    // Select correct interrupt for the UART we are using
    int UART_IRQ = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;

    // And set up and enable the interrupt handlers
    irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
    irq_set_enabled(UART_IRQ, true);

    // Now enable the UART to send interrupts - RX only
    uart_set_irq_enables(UART_ID, true, false);

    // OK, all set up.
    // Lets send a basic string out, and then run a loop and wait for RX interrupts
    // The handler will count them, but also reflect the incoming data back with a slight change!
    uart_puts(UART_ID, "\nHello, uart interrupts\n");

return PICO_OK;
}