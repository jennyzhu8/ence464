#include <FreeRTOS.h>
#include <task.h>
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/adc.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/pwm.h"
#include "driverlib/sysctl.h"
#include "utils/uartstdio.h"
#include "driverlib/rom.h"
#include "driverlib/uart.h"

#define BUF_SIZE 10

// PWM configuration
#define PWM_START_RATE_HZ  80
#define PWM_RATE_STEP_HZ   50
#define PWM_RATE_MIN_HZ    50
#define PWM_RATE_MAX_HZ    400
#define PWM_FIXED_DUTY     85
#define PWM_DIVIDER_CODE   SYSCTL_PWMDIV_4
#define PWM_DIVIDER        4
//  PWM Hardware Details M0PWM7 (gen 3)
//  ---Main Rotor PWM: PC5, J4-05
#define PWM_MAIN_BASE        PWM0_BASE
#define PWM_MAIN_GEN         PWM_GEN_3
#define PWM_MAIN_OUTNUM      PWM_OUT_7
#define PWM_MAIN_OUTBIT      PWM_OUT_7_BIT
#define PWM_MAIN_PERIPH_PWM  SYSCTL_PERIPH_PWM0
#define PWM_MAIN_PERIPH_GPIO SYSCTL_PERIPH_GPIOC
#define PWM_MAIN_GPIO_BASE   GPIO_PORTC_BASE
#define PWM_MAIN_GPIO_CONFIG GPIO_PC5_M0PWM7
#define PWM_MAIN_GPIO_PIN    GPIO_PIN_5

/**
 * main.c
 */

static void NullTaskFunc(void *);
static void Blink_LED_task(void *);
static void ADC_task(void *);
void initialisePWM (void);
void setPWM (uint32_t u32Freq, uint32_t u32Duty);
void initialiseLED (void);
void initialiseADC (void);

void
ConfigureUART(void)
{
    //
    // Enable the GPIO Peripheral used by the UART.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //
    // Enable UART0
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    //
    // Configure GPIO Pins for UART mode.
    //
    ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
    ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Use the internal 16MHz oscillator as the UART clock source.
    //
    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);

    //
    // Initialize the UART for console I/O.
    //
    UARTStdioConfig(0, 115200, 16000000);
}

void
initialisePWM (void)
{
    SysCtlPeripheralEnable(PWM_MAIN_PERIPH_PWM);
    SysCtlPeripheralEnable(PWM_MAIN_PERIPH_GPIO);

    GPIOPinConfigure(PWM_MAIN_GPIO_CONFIG);
    GPIOPinTypePWM(PWM_MAIN_GPIO_BASE, PWM_MAIN_GPIO_PIN);

    PWMGenConfigure(PWM_MAIN_BASE, PWM_MAIN_GEN,
                    PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_NO_SYNC);
    // Set the initial PWM parameters
    setPWM (PWM_START_RATE_HZ, PWM_FIXED_DUTY);

    PWMGenEnable(PWM_MAIN_BASE, PWM_MAIN_GEN);

    // Disable the output.  Repeat this call with 'true' to turn O/P on.
    PWMOutputState(PWM_MAIN_BASE, PWM_MAIN_OUTBIT, false);

}

/********************************************************
 * Function to set the freq, duty cycle of M0PWM7
 ********************************************************/
void
setPWM (uint32_t ui32Freq, uint32_t ui32Duty)
{
    // Calculate the PWM period corresponding to the freq.
    uint32_t ui32Period = SysCtlClockGet() / PWM_DIVIDER / ui32Freq;

    PWMGenPeriodSet(PWM_MAIN_BASE, PWM_MAIN_GEN, ui32Period);
    PWMPulseWidthSet(PWM_MAIN_BASE, PWM_MAIN_OUTNUM, ui32Period * ui32Duty / 100);
}

void
initialiseLED (void)
{
    // For LED blinky task - initialize GPIO port F and then pin #1 (red) for output
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF)) ; // busy-wait until GPIOF's bus clock is ready

    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1); // PF_1 as output
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2); // PF_2 as output
    // doesn't need too much drive strength as the RGB LEDs on the TM4C123 launchpad are switched via N-type transistors

    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD);
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0); // off by default
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD);
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0); // off by default

}

void
initialiseADC (void)
{
    //
    // Enable the ADC0 module.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

    // Wait for the ADC0 module to be ready.
    //
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0))
    {
    }
    //
    // Enable the first sample sequencer to capture the value of channel 0 when
    // the processor trigger occurs.
    //
    ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PROCESSOR, 0);
    ADCSequenceStepConfigure(ADC0_BASE, 0, 0,ADC_CTL_IE | ADC_CTL_END | ADC_CTL_CH9);
    ADCSequenceEnable(ADC0_BASE, 0);

}


int main(void)
{

    // Set the clock rate to 80 MHz
    SysCtlClockSet (SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    ConfigureUART();
    initialisePWM();
    initialiseLED();
    initialiseADC();

    PWMOutputState(PWM_MAIN_BASE, PWM_MAIN_OUTBIT, true);


    if (pdTRUE != xTaskCreate(Blink_LED_task, "Blinker Blue", 32, (void *)2, 4, NULL))
    { // (void *)2 is our pvParameters for our task func specifying PF_2
         while (1) ; // error creating task, out of memory?
    }

    if (pdTRUE != xTaskCreate(NullTaskFunc, "Null Task", 32, NULL, 4, NULL))
    {
        while(1); // Oh no! Must not have had enough memory to create task.
    }

    if (pdTRUE != xTaskCreate(ADC_task, "ADC Task", 32, (void *)4, 1, NULL))
    {
         while (1) ; // error creating task, out of memory?
    }

    vTaskStartScheduler(); // Start FreeRTOS!!

    // Should never get here since the RTOS should never "exit".
    while(1);

}

//Blinky function

void Blink_LED_task(void *pvParameters) {
    /* While pvParameters is technically a pointer, a pointer is nothing
    * more than an unsigned integer of size equal to the architecture's
    * memory address bus width, which is 32-bits in ARM. We're abusing
    * the parameter then to hold a simple integer value. Could also have
    * used this as a pointer to a memory location holding the value, but
    * our method uses less memory.
    */
    const unsigned int whichLed = (unsigned int)pvParameters;
    // TivaWare GPIO calls require the pin# as a binary bitmask,
    // not a simple number. Alternately, we could have passed the
    // bitmask into pvParameters instead of a simple number.
    const uint8_t whichBit = 1 << whichLed;
    uint8_t currentValue = 0;
    while (1)
    {
        // XOR toggles the bit on/off each time this runs.
        currentValue ^= whichBit;
        GPIOPinWrite(GPIO_PORTF_BASE, whichBit, currentValue);

        // Suspend this task (so others may run) for 125ms
        // or as close as we can get with the current RTOS tick setting.
        // (vTaskDelay takes scheduler ticks as its parameter, so use the
        // pdMS_TO_TICKS macro to convert milliseconds to ticks.)
        vTaskDelay(pdMS_TO_TICKS(125));
    }
    // No way to kill this blinky task unless another task has an
    // xTaskHandle reference to it and can use vTaskDelete() to purge it.
}

static void ADC_task(void *pvParameters)
{
    uint16_t i;
    uint32_t ui32Value;
    uint32_t alt_buffer[BUF_SIZE];
    uint8_t index = 0;
    int32_t alt = 0;
    uint32_t height;


    while(1)
    {
        //
        // Trigger the sample sequence.
        //
        ADCProcessorTrigger(ADC0_BASE, 0);
        //
        // Wait until the sample sequence has completed.
        //
        while(!ADCIntStatus(ADC0_BASE, 0, false))
        {
        }
        //
        // Read the value from the ADC.
        //
        ADCSequenceDataGet(ADC0_BASE, 0, &ui32Value);
        alt_buffer[index] = ui32Value;
        index = (index + 1) % BUF_SIZE;
        alt = 0;
        for (i=0; i < BUF_SIZE; i++) {
            alt = alt + alt_buffer[i];
        }
        alt = alt / BUF_SIZE;
        // Values depend on the rig you are using
        // Min Altitude = 2230, Max Altitude = 1000
        // For emulator to 100%
        // Min Alt = 3030, Max alt = 1900
        // Emulator to max which is 133%
        // Min alt =3030, max alt = 1330


        //getting height between 1660 and 0
        height = alt-1330;
        //getting height as a percentage
        height =  (133 - ((height*133)/1700));

        UARTprintf("Altitude = %d ", height);
        vTaskDelay(200);

    }
}
static void NullTaskFunc(void *pvParameters)
{
    while(1)
    {
        // With this task always delaying, the RTOS Idle Task runs almost
        // all the time.
        vTaskDelay(10000);
    }
}


// This is an error handling function called when FreeRTOS asserts.
// This should be used for debugging purposes
void vAssertCalled( const char * pcFile, unsigned long ulLine ) {
    (void)pcFile; // unused
    (void)ulLine; // unused
    while (true) ;
}
