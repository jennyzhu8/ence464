/*
 * PID_task.c
 *
 *  Created on: 9/08/2023
 *      Author: bth74
 */

#include <stdint.h>
#include <stdbool.h>
#include "PID_controller.h"


// **************************************************************
// pid_init: Initialises a PID struct based on the input gains and the accumulator limit
//  (Initial Setter)
void
pid_init(PID* pid, uint8_t kp, uint8_t ki, uint8_t kd, int16_t accum_limit, int16_t max_output)
{
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->accumulator = 0;
    pid->error = 0;
    pid->derivative = 0;
    pid->limit = accum_limit;
    pid->max_output = max_output;
}

// **************************************************************
// pid_update: Updates a PID object based on the input error and change in time from the last call
//  (Setter)
void
pid_update(PID* pid, int16_t error, int16_t dt)
{
    // Add the error to the integral
    pid->accumulator += error*dt;

    // Clamp the accumulator to the specified limit
    if (pid->accumulator > pid->limit) pid->accumulator = pid->limit;
    if (pid->accumulator < -pid->limit) pid->accumulator = -pid->limit;

    // Calculate the derivative
    pid->derivative = (error - pid->error)/ dt;
    // Store the current error
    pid->error = error;
}

// **************************************************************
// pid_get_command: Gets the PID control from the PID object
//  (Getter)
float
pid_get_command(PID* pid)
{
    float value = (pid->kp*pid->error) + (pid->ki*pid->accumulator) + (pid->kd*pid->derivative);
    if (value > pid->max_output) { value = pid->max_output; }
    if (value < -pid->max_output) { value = -pid->max_output; }
    return value;
}

// **************************************************************
// pid_reset: Resets the PID for testing purposes.
//  (Setter)
void
pid_reset(PID* pid)
{
    pid->accumulator = 0;
    pid->derivative = 0;
}
