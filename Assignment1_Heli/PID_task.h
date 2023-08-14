/*
 * PID_task.h
 *
 *  Created on: 9/08/2023
 *      Author: bth74
 */

// PWM configuration
#define PWM_START_RATE_HZ  250

#ifndef PID_TASK_H_
#define PID_TASK_H_

typedef struct PID PID;



void PID_Task(void *pvParameters);

void pid_init(PID* pid, uint8_t kp, uint8_t ki, uint8_t kd, int16_t accum_limit);
void pid_update(PID* pid, int16_t error, int16_t dt);
float pid_get_command(PID* pid);
void control_init(void);
void control_update(int16_t alt_error, int16_t yaw_error);

#endif /* PID_TASK_H_ */
