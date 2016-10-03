#ifndef __SERVO_H
#define __SERVO_H

#include "stm32l476xx.h"
#include "SysClock.h"
#include <string.h>
#include <stdio.h>

#define NUM_MOTORS (2)
#define MAX_RECIPE_SIZE (100)

/* opcode lookups */
#define MOV (32)
#define WAIT (64)
#define LOOP_START (128)
#define END_LOOP (160)
#define RECIPE_END (0)

/* masks for reading recipe commands */
#define OPCODE_MASK (224) //0b11100000
#define PARAMETER_MASK (31) //0b00011111

/* enumeration of the 6 possible positions */
typedef enum {
	POS_0,
	POS_1,
	POS_2,
	POS_3,
	POS_4,
	POS_5
} position;

/* enumeration of running/paused state */
typedef enum {
	PAUSED,
	RUNNING
} status;

/* struct to represent each motor's data */
typedef struct{
	uint8_t instruction_index;
	int16_t loop_count;
	uint8_t in_loop;
	uint8_t loop_index;
	position pos;
	status status;
} servo_data;

/* variable pulse widths for the motor positions */
uint8_t pos_pulse_widths[POS_5+1] = {4, 7, 10, 13, 16, 19};

/* test recipe */
uint8_t recipe[2][MAX_RECIPE_SIZE] = {

	{ MOV+0,
	MOV+5,
	MOV+0,
	MOV+3,
	LOOP_START+0,
	MOV+1,
	MOV+4,
	END_LOOP,
	MOV+0,
	MOV+2,
	WAIT+0,
	MOV+3,
	WAIT+0,
	MOV+2,
	MOV+3,
	WAIT+31,
	WAIT+31,
	WAIT+31,
	MOV+4,
	RECIPE_END},
	
	{ MOV+0,
	MOV+5,
	MOV+0,
	MOV+3,
	LOOP_START+0,
	MOV+1,
	MOV+4,
	END_LOOP,
	MOV+0,
	MOV+2,
	WAIT+0,
	MOV+3,
	WAIT+0,
	MOV+2,
	MOV+3,
	WAIT+31,
	WAIT+31,
	WAIT+31,
	MOV+4,
	RECIPE_END}
};

#endif /* __SERVO_H */
