#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include <pico/binary_info.h>

#include "hardware/uart.h"
#include "hardware/irq.h"
#include "gps.h"
#include "uart.h"
#include "main_gps_uart_shared_buffer.h"

char buffer_UART[BUFLEN] = {0}; // Define the buffer
char buffer_GPS[BUFLEN] = {0}; // Define the buffer for GPS data

int chars_rxed = 0; // Global variable to track the number of characters received

#ifndef PICO
// Ensure that PICO_RP2350A is defined to 0 for PICUBED builds.
// boards/samwise_picubed.h should define it to 0.
// The CMakeLists.txt file points to this file for the board definition.
    #ifdef PICO_RP2350A
        static_assert(PICO_RP2350A == 0, "PICO_RP2350A must be undefined or 0 for PICUBED builds.");
    #endif
#endif

//Check the pin is compatible with the platform
#if UART_RX_PIN >= NUM_BANK0_GPIOS
#error "Recompile specifying the RP2350B platform SAMWISE"
#endif

// Set binary information for 'picotool info -a' (see https://github.com/raspberrypi/picotool#readme)
bi_decl(bi_program_description("Let's test a UART in loopback mode with GPS data parsing."));
bi_decl(bi_program_name("PiUART - GPS and UART Loopback Example"));
bi_decl(bi_program_url("https://github.com/pete4radio/PiUART"));
bi_decl(bi_program_version_string("Build date and time: " __DATE__ " " __TIME__));
bi_decl(bi_2pins_with_func(UART_RX_PIN, UART_TX_PIN, GPIO_FUNC_UART));

int main() {
    extern int chars_rxed;
    stdio_init_all();
    sleep_ms(5000); // Wait for the serial port to be ready

//  UART
    absolute_time_t previous_time_UART = get_absolute_time();     // ms
    uint32_t interval_UART = 500000;
    char buffer_UART[BUFLEN];
    buffer_UART[0] = 0x00; //  Initialize the buffer to empty

// GPS
    absolute_time_t previous_time_GPS = get_absolute_time();     // ms
    uint32_t interval_GPS = 500000;
    extern char buffer_GPS[BUFLEN];
    buffer_GPS[0] = 0x00; //  Initialize the buffer to empty
    gps_data_t *gps_data = malloc(sizeof(gps_data_t));
    if (!gps_data) {
        printf("ERROR: Failed to allocate memory for gps_data\n");
    }

    printf("Hello, GPS and UART!\n");
    init_uart(); // Initialize the UART

    while (true) {
// Time to UART?
        if (absolute_time_diff_us(previous_time_UART, get_absolute_time()) >= interval_UART) {
            // Save the last time you blinked checked UART loopback
            previous_time_UART = get_absolute_time();
        //  First byte of buffer is zero until the interrupt routine provides a complete line
            sprintf(buffer_UART, "");
            if (init_uart() == PICO_OK) {
                //  If we got a complete line, print it
                if (buffer_UART[0] != '\0') {
                    sprintf("UART: %s", buffer_UART);
                }
            } else {
                sprintf(buffer_UART, "UART: not found\n");
            }
        }


// Time to GPS?
        if (absolute_time_diff_us(previous_time_GPS, get_absolute_time()) >= interval_GPS) {
            // Save the last time you blinked checked GPS
            previous_time_GPS = get_absolute_time();
            //  Is there a line for us to decode?
            if (buffer_UART[0] != '\0'  && (gps_data != NULL)) {
                printf("UART: %s\n", buffer_UART);
                //  Decode the GPS data from the UART buffer
                do_gps(buffer_UART, gps_data);
                chars_rxed = 0; // Clear the UART buffer after processing to accept another GPS sentence
                buffer_UART[0] = '\0'; // Reset the UART buffer for the next read
                //  Print the GPS data
                if (gps_data->has_fix) {
                    sprintf(buffer_GPS, "GPS Fix: %d, Lat: %.6f %c, Lon: %.6f %c, Alt: %.2f m, Speed: %.2f knots\n",
                            gps_data->fix_quality,
                            gps_data->latitude, gps_data->lat_dir,
                            gps_data->longitude, gps_data->lon_dir,
                            gps_data->altitude_m,
                            gps_data->speed_knots);
                } else {
                    sprintf(buffer_GPS, "GPS No Fix\n");
                }
//  If we're in loopback, this will repeat the cycle.
                if (UART_ID != NULL && uart_is_writable(UART_ID)) {
                    uart_puts(UART_ID, "\nHello, uart interrupts\n"); 
                    } else {
                        printf("Could not write to UART for GPS loopback.\n");
                    }
            } else {
                sprintf(buffer_GPS, "GPS\n");
            }
        printf("%d, %s\n", chars_rxed, buffer_GPS);
        }
        sleep_ms(10);
    }
}
