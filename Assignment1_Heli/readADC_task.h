/*
 * readADC_task.h
 *
 *  Created on: 1/08/2023
 *      Author: bth74
 */

#define BUF_SIZE 5

#ifndef READADC_TASK_H_
#define READADC_TASK_H_



void initialiseADC (void);
void altitude_ADC_task(void *pvParameters);


#endif /* READADC_TASK_H_ */
