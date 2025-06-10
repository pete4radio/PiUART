#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "uart.h"
#include "main_gps_uart_shared_buffer.h"


// RX interrupt handler
void on_uart_rx() {
    extern int chars_rxed;
    extern char
    while (uart_is_readable(UART_ID)) {
        uint8_t ch = uart_getc(UART_ID);

        // If buffer is full (or CR LF has been stored), drop new characters
        if (chars_rxed > BUFLEN - 1) {
                buffer_UART[BUFLEN - 1] = '\0'; // Null-terminate at the end
                return; 
            }

        // If newline, terminate string and exit handler
        if (ch == '\n' || ch == '\r') {
            buffer_UART[chars_rxed] = '\0'; // Null-terminate at current position
            chars_rxed = BUFLEN; // next characters are tossed until progrm processes this sentence
            return;
        }

        // Otherwise, store character and increment counter
        buffer_UART[chars_rxed++] = ch;
    }
}

uint8_t init_uart() {
    // Initialize the UART, and set up the pins
    
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