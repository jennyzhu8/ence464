/*
 * PID_controller.h
 *
 *  Created on: 9/08/2023
 *      Author: bth74
 */
#ifndef PID_CONTROLLER_H_
#define PID_CONTROLLER_H_

#include <stdint.h>

typedef struct PID {
    int16_t accumulator;
    int16_t limit;
    int16_t error;
    int16_t derivative;
    uint8_t kp, ki, kd;
} PID;

void pid_init(PID* pid, uint8_t kp, uint8_t ki, uint8_t kd, int16_t accum_limit);
void pid_update(PID* pid, int16_t error, int16_t dt);
float pid_get_command(PID* pid);

#endif /* PID_CONTROLLER_H_ */
