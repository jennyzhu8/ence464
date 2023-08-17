/*
 * PID_task.c
 *
 *  Created on: 9/08/2023
 *      Author: bth74
 */

#include <stdint.h>
#include <stdbool.h>
#include "PID_controller.h"

#define CONTROL_DT (1.f/50.f)

// Initialises the PID controller
void pid_init(PID* pid, uint8_t kp, uint8_t ki, uint8_t kd, int16_t accum_limit) {
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->accumulator = 0;
    pid->error = 0;
    pid->derivative = 0;
    pid->limit = accum_limit;

}

void pid_update(PID* pid, int16_t error, int16_t dt) {
    // Add the error to the integral
    pid->accumulator += error*dt;

    // Clamp the accumulator to the specified limit
    if (pid->accumulator > pid->limit) pid->accumulator = pid->limit;
    if (pid->accumulator < pid->limit) pid->accumulator = -pid->limit;

    // Calculate the derivative
    pid->derivative = (error - pid->error)/ dt;
    // Store the current error
    pid->error = error;
}

float pid_get_command(PID* pid) {
    return pid->kp * pid->error + pid->ki * pid->accumulator + pid->kd * pid->derivative;
}
