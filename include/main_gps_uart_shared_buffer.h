//  Main creates the storage for these buffers.
//  The uart interrupt handler puts the string into the UART buffer,
//  And the GPS reads that line,and converts it to the GPS data
//  Also main can see and manipulate the buffers as well.

#ifndef BUFLEN
#define BUFLEN 160
#endif

extern char buffer_UART[BUFLEN];
extern char buffer_GPS[BUFLEN];
extern char buffer_DEBUG[BUFLEN * 10]; // Buffer for debug output
extern int write_here; // Global variable to track the position in the debug buffer
extern int chars_rxed; // Global variable to track how many characters have been received
extern int lfcr_rxed; // Global variable to track how many LF or CR has been received   

    //  storing has three states:  
    //  0 = not storing because no $ yet, 1 = storing in progress, 2 = waiting for  program to ACK
extern int storing; // Add this as a global or static variable, initialized to 0