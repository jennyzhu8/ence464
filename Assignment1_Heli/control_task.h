/*
 * control_task.h
 *
 *  Created on: 17/08/2023
 *      Author: hjo48
 */
#ifndef CONTROL_TASK_H_
#define CONTROL_TASK_H_

// Stepping size from 0-100%, step size of 10% results in 11 separate values to the set the motor to
#define STEP_SIZE_HEIGHT 10

// PWM configuration
#define PWM_START_RATE_HZ  250
#define PWM_MID_VALUE 53
#define PWM_MAX 73
#define PWM_MIN 23

void Control_Task(void *pvParameters);
void initialiseControl(void);
void control_update(int16_t alt_error, int16_t yaw_error);

#endif /* CONTROL_TASK_H_ */
