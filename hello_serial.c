#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include <pico/binary_info.h>
#include <string.h>

#include "hardware/uart.h"
#include "hardware/irq.h"
#include "gps.h"
#include "uart.h"
#include "main_gps_uart_shared_buffer.h"

char buffer_UART[BUFLEN] = {0}; // Define the buffer
char buffer_GPS[BUFLEN] = {0}; // Define the buffer for GPS data
char buffer_DEBUG[BUFLEN * 10] = {0}; // Buffer for debug output

int chars_rxed = 0; // Global variable to track the number of characters received
int lfcr_rxed = 0; // Global variable to track how many LF or CR has been received
int write_here = 0; // Global variable to track the position in the debug buffer
int storing = 0; // Global variable to track if we are currently storing data
int zeros_rxed = 0; // Global variable to track how many null characters have been received

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
    extern char buffer_UART[BUFLEN];
    extern char buffer_DEBUG[BUFLEN * 10]; // Buffer for debug output
    extern int write_here; // Global variable to track the position in the debug buffer
    //  storing has three states:  
    //  0 = not storing because no $ yet, 1 = storing in progress, 2 = waiting for  program to ACK
    extern int storing; // Add this as a global or static variable, initialized to 0
    extern int lfcr_rxed; // Global variable to track how many LF or CR has been received
    extern int zeros_rxed;
    stdio_init_all();
    sleep_ms(5000); // Wait for the serial port to be ready

//  UART
    absolute_time_t previous_time_UART = get_absolute_time();     // ms
    uint32_t interval_UART = 50000;
    //char buffer_UART[BUFLEN];
    buffer_UART[0] = 0x00; //  Initialize the buffer to empty

// GPS
    absolute_time_t previous_time_GPS = get_absolute_time();     // ms
    uint32_t interval_GPS = 50000;
    extern char buffer_GPS[BUFLEN];
    buffer_GPS[0] = 0x00; //  Initialize the buffer to empty
    gps_data_t *gps_data = malloc(sizeof(gps_data_t));
    if (!gps_data) {
        printf("ERROR: Failed to allocate memory for gps_data\n");
    }
    //memset(&gps_data, 0, sizeof(gps_data));

    printf("Hello, GPS and UART!\n");
    init_uart(); // Initialize the UART
    // Start at the beginning of the buffer
    chars_rxed = 0;
    lfcr_rxed = 0; // Initialize the LF/CR counter
    zeros_rxed = 0;
    storing = 0; // Initialize storing to 0 (not storing yet, need a $ to start storing)
    write_here = 0; // Initialize the write position in the debug buffer

    while (true) {
// Time to UART?
        if (absolute_time_diff_us(previous_time_UART, get_absolute_time()) >= interval_UART) {
            // Save the last time you blinked checked UART loopback
            previous_time_UART = get_absolute_time();
            if (init_uart() == PICO_OK) {
                //  If we got a complete line, print it
                if (storing == 2) {   // signals <CR> or <LF> has been received
                    printf("UART Complete: %s\n", buffer_UART);
                    printf("UART: %d lost sentences;  %d characters received; %d zeros received.\n", 
                        lfcr_rxed, chars_rxed, zeros_rxed);
                    lfcr_rxed = 0; // Reset the LF/CR counter after processing a complete line
                    zeros_rxed = 0; // Reset the zeros counter after processing a complete line
                }
            } else {
                printf("UART: not found\n");
            }
        }


// Time to GPS?
        if (absolute_time_diff_us(previous_time_GPS, get_absolute_time()) >= interval_GPS) {
            // Save the last time you blinked checked GPS
            previous_time_GPS = get_absolute_time();
            //  Is there a line for us to decode?
            if ((storing == 2)  && (gps_data != NULL)) {
                //  Decode the GPS data from the UART buffer
                if (do_gps(buffer_UART, gps_data) == PICO_ERROR_GENERIC) {
                    printf("GPS: Error parsing NMEA sentence: %s\n", buffer_UART);
                }
                storing = 0; // Reset storing to 0 to indicate we are ready to receive a new sentence
                chars_rxed = 0; // Clear the UART buffer after processing to accept another GPS sentence
                continue; // Skip further processing for this iteration
                chars_rxed = 0; // Clear the UART buffer after processing to accept another GPS sentence
                storing = 0; // Set storing to 0 to indicate we are ready to receive a new sentence
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
                sprintf(buffer_GPS, "GPS");
            }
        printf("%d, %s\n", chars_rxed, buffer_GPS);
        }
        sleep_ms(10);
    }
}
