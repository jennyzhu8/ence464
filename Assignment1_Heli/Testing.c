/*
 * Testing.c
 *
 * Created on: 17/08/23
 * Author: Harrison Johnson
 *
 */

#include <stdint.h>
#include <stdbool.h>

#include "inc/hw_memmap.h"
#include "inc/hw_gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "utils/uartstdio.h"
#include "driverlib/rom.h"
#include "driverlib/uart.h"

#include "PID_controller.h"
// ***********************************************
// Globals and definitions

#define PID_DT (1.f/50.f)
PID g_test_PID;

void ConfigureUART(void);
void test_PID(void);
void assert(bool);

// ***********************************************
// ConfigureUART: Sets up the UART serial interface
void
ConfigureUART(void)
{
    // Enable the GPIO Peripheral used by the UART.
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    // Enable UART0
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    // Configure GPIO Pins for UART mode.
    ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
    ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // Use the internal 16MHz oscillator as the UART clock source.
    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);

    // Initialize the UART for console I/O.
    UARTStdioConfig(0, 115200, 16000000);
}

// ***********************************************
// assert: Similar replica assert function
static uint8_t fail_counter = 0;
static uint8_t pass_counter = 0;

void
assert (bool assertion)
{
    if (assertion) {
        pass_counter += 1;
        UARTprintf("PASS\n");
    } else {
        fail_counter += 1;
        UARTprintf("FAIL\n");
    }
}

// ***********************************************
// test_controller: Black-Box test of the PID controller
void
test_PID (void)
{
    //Initialises a PID to test on. Imitates the conditions of the real PID
    pid_init(&g_test_PID, 1, 1, 0, 100, 20);
    int16_t error;
    int16_t value;

    //Test case 1:
    error = -100;
    pid_reset(&g_test_PID);
    pid_update(&g_test_PID, error, PID_DT);
    value = (int)pid_get_command(&g_test_PID);
    assert(value == -20);

    //Test case 2:
    error = -20;
    pid_reset(&g_test_PID);
    pid_update(&g_test_PID, error, PID_DT);
    value = (int)pid_get_command(&g_test_PID);
    assert((value >= -20 ) && (value < 0));

    //Test case 3:
    error = 0;
    pid_reset(&g_test_PID);
    pid_update(&g_test_PID, error, PID_DT);
    value = (int)pid_get_command(&g_test_PID);
    assert(value == 0);

    //Test case 4:
    error = 20;
    pid_reset(&g_test_PID);
    pid_update(&g_test_PID, error, PID_DT);
    value = (int)pid_get_command(&g_test_PID);
    assert((value <= 20 ) && (value > 0));

    //Test case 5:
    error = 100;
    pid_reset(&g_test_PID);
    pid_update(&g_test_PID, error, PID_DT);
    value = (int)pid_get_command(&g_test_PID);
    assert(value == 20);
}

// ***********************************************
// MAIN:
void
main (void)
{
    ConfigureUART();
    test_PID();

    UARTprintf("%d Tests Completed\n", (fail_counter+pass_counter));
    UARTprintf("Passed: %d\n", pass_counter);
    UARTprintf("Failed: %d\n\n", fail_counter);

    while(1)
    {
    }
}


// ***********************************************
// NOT USED HERE
// This is an error handling function called when FreeRTOS asserts.
// This should be used for debugging purposes
void vAssertCalled( const char * pcFile, unsigned long ulLine ) {
    (void)pcFile; // unused
    (void)ulLine; // unused
    while (true) ;
}
