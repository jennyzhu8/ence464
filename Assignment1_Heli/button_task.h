/*
 * button_task.h
 *
 *  Created on: 9/08/2023
 *      Author: bth74
 */

// Buttons (from buttons4.h which we should use instead of buttons.h)
// UP button
#define UP_BUT_PERIPH  SYSCTL_PERIPH_GPIOE
#define UP_BUT_PORT_BASE  GPIO_PORTE_BASE
#define UP_BUT_PIN  GPIO_PIN_0
#define UP_BUT_NORMAL  false
// DOWN button
#define DOWN_BUT_PERIPH  SYSCTL_PERIPH_GPIOD
#define DOWN_BUT_PORT_BASE  GPIO_PORTD_BASE
#define DOWN_BUT_PIN  GPIO_PIN_2
#define DOWN_BUT_NORMAL  false

#ifndef BUTTON_TASK_H_
#define BUTTON_TASK_H_

void initialiseButtons(void);
void ButtonTask(void *pvParameters);
void Button_LED_Task(void *pvParameters);



#endif /* BUTTON_TASK_H_ */
