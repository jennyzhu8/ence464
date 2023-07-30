/*
 * pwm-oled
 *
 * Harry Mander 2022
 *
 * Demo application showing use of buttons, OLED display, and PWM outputs with
 * FreeRTOS. Press UP/DOWN buttons to select PWM output, then press LEFT/RIGHT
 * buttons to decrease/increase duty cycle of the currently selected output.
 */

#include <stdbool.h>
#include <stdint.h>

#include <driverlib/pin_map.h>
#include <driverlib/sysctl.h>
#include <driverlib/gpio.h>
#include <driverlib/pwm.h>
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <utils/ustdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include "buttons.h"
#include "OrbitOLED/OrbitOLEDInterface.h"

#define MAX_DC 95
#define MIN_DC 5
#define STEP_DC 5
#define INIT_DC 50

static QueueHandle_t button_q;

static void button_task(void *arg)
{
    (void) arg;
    TickType_t wake_time = xTaskGetTickCount();

    initButtons();

    while (1) {
        updateButtons();

        for (enum butNames button = 0; button < NUM_BUTS; button++) {
            if (checkButton(button) == PUSHED)
                xQueueSend(button_q, &button, portMAX_DELAY);
        }

        vTaskDelayUntil(&wake_time, pdMS_TO_TICKS(20));
    }
}

struct pwm_config {
    uint32_t pwm_periph;
    uint32_t pwm_gen;
    uint32_t pwm_base;
    uint32_t outnum;
    uint32_t outbit;

    uint32_t gpio_periph;
    uint32_t gpio_base;
    uint32_t gpio_config;
    uint32_t gpio_pin;

    const char *pin_name;

    uint8_t duty;
    uint32_t frequency;
};

#define PWM(mod, gen, out, port, pin, freq) (struct pwm_config) { \
    .pwm_periph = SYSCTL_PERIPH_PWM##mod, \
    .pwm_gen = PWM_GEN_##gen, \
    .pwm_base = PWM##mod##_BASE, \
    .outnum = PWM_OUT_##out, \
    .outbit = PWM_OUT_##out##_BIT, \
    .gpio_periph = SYSCTL_PERIPH_GPIO##port, \
    .gpio_base = GPIO_PORT##port##_BASE, \
    .gpio_config = GPIO_P##port##pin##_M##mod##PWM##out, \
    .gpio_pin = GPIO_PIN_##pin, \
    .pin_name = "P" #port #pin, \
    .duty = 0, \
    .frequency = freq, \
}

#define PWM_DIVIDER 4
#define PWM_DIVIDER_CODE SYSCTL_PWMDIV_4

static struct pwm_config pwms[] = {
    PWM(0, 1, 3, B, 5, 300),
    PWM(0, 2, 4, E, 4, 400),
    PWM(0, 3, 7, C, 5, 500),
    PWM(1, 1, 2, A, 6, 600),

    // Note PF2 and PF3 share the same generator so must have the same frequency
    PWM(1, 3, 6, F, 2, 700),
    PWM(1, 3, 7, F, 3, 700),
};

#define NUM_PWMS (sizeof(pwms) / sizeof(pwms[0]))

static inline void pwm_set_duty(struct pwm_config *pwm, uint8_t duty)
{
    PWMPulseWidthSet(pwm->pwm_base, pwm->outnum,
        SysCtlClockGet() / PWM_DIVIDER / pwm->frequency * duty / 100);
    pwm->duty = duty;
}

static inline void enable_periph(uint32_t p)
{
    if (!SysCtlPeripheralReady(p)) {
        SysCtlPeripheralEnable(p);
        while (!SysCtlPeripheralReady(p)) {}
    }
}

static inline void pwm_config_init(struct pwm_config *pwm)
{
    SysCtlPWMClockSet(PWM_DIVIDER_CODE);

    enable_periph(pwm->pwm_periph);
    enable_periph(pwm->gpio_periph);

    GPIOPinConfigure(pwm->gpio_config);
    GPIOPinTypePWM(pwm->gpio_base, pwm->gpio_pin);

    PWMGenConfigure(pwm->pwm_base, pwm->pwm_gen,
                    PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_NO_SYNC);
    PWMGenPeriodSet(pwm->pwm_base, pwm->pwm_gen,
        SysCtlClockGet() / PWM_DIVIDER / pwm->frequency);
    pwm_set_duty(pwm, INIT_DC);
    PWMGenEnable(pwm->pwm_base, pwm->pwm_gen);

    PWMOutputState(pwm->pwm_base, pwm->outbit, true);
}

static void pwm_init(void)
{
    for (unsigned int i = 0; i < sizeof(pwms) / sizeof(pwms[0]); i++)
        pwm_config_init(&pwms[i]);
}

static void oled_update_pwm_duty(const struct pwm_config *pwm)
{
    char str[12];
    usnprintf(str, sizeof(str), "DC: %3u%%", pwm->duty);
    OLEDStringDraw(str, 0, 1);
}

static void oled_update_pwm(const struct pwm_config *pwm)
{
    char str[12];

    usnprintf(str, sizeof(str), "PWM #%u", pwm - pwms + 1);
    OLEDStringDraw(str, 0, 0);

    usnprintf(str, sizeof(str), "%3u Hz", pwm->frequency);
    OLEDStringDraw(str, 0, 2);

    OLEDStringDraw(pwm->pin_name, 0, 3);
    oled_update_pwm_duty(pwm);
}

static void pwm_task(void *arg)
{
    (void) arg;

    uint8_t current_output = 0;

    pwm_init();
    OLEDInitialise();
    oled_update_pwm(&pwms[current_output]);

    while (1) {
        enum butNames button;
        const uint8_t old_output = current_output;
        const uint8_t old_duty = pwms[current_output].duty;
        uint8_t duty = old_duty;

        if (xQueueReceive(button_q, &button, portMAX_DELAY) == pdTRUE) {
            switch (button) {
            case UP:
                current_output = (current_output + 1) % NUM_PWMS;
                break;
            case DOWN:
                if (current_output)
                    current_output -= 1;
                else
                    current_output = NUM_PWMS - 1;
                break;
            case LEFT:
                if (duty < MIN_DC + STEP_DC)
                    duty = MAX_DC - (duty - STEP_DC);
                else
                    duty -= STEP_DC;
                break;
            case RIGHT:
                if (duty + STEP_DC > MAX_DC)
                    duty = MAX_DC - duty + MIN_DC;
                else
                    duty += STEP_DC;
                break;
            default:
                break;
            }

            if (current_output != old_output) {
                oled_update_pwm(&pwms[current_output]);
            } else if (duty != old_duty) {
                pwm_set_duty(&pwms[current_output], duty);
                oled_update_pwm_duty(&pwms[current_output]);
            }
        }
    }
}

int main(void)
{
    SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL
        | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    button_q = xQueueCreate(10, sizeof(enum butNames));

    xTaskCreate(pwm_task, "pwm_task", 256, NULL, 0, NULL);
    xTaskCreate(button_task, "button_task", 256, NULL, 0, NULL);

    vTaskStartScheduler();

    return 0;
}

void vAssertCalled( const char * pcFile, unsigned long ulLine )
{
    (void) pcFile;
    (void) ulLine;
    while (1) ;
}

