## Overview

Design and implement a STM32 Discovery board program exhibiting multitasking
characteristics in simultaneously controlling a pair of servo motors using a custom
interpreted control language. The system will be responsive to simultaneous
independent, externally provided commands. The servo positions are controlled with
pulse-width modulation (PWM). This first portion of the project utilizes the STM32
Discovery board. Each servo will be controlled by its own recipe of commands in the
custom interpreted control language. 

## Servo and PWM Overview

The servos can be positioned to any of six positions (0, 1, 2, 3, 4, 5) approximately
evenly spaced along the servos’ potential 160 degree arc. Position 0 is at the
extreme clockwise position, and Position 5 is at the extreme counterclockwise
position.

Typical Futaba position control servos require a PWM control signal with a 20
millisecond period. The duty cycle of this PWM signal controls the servo through its
range of positions. The typical duty cycle ranges from 2% to over 10% for full range
of positions. 

## Command Language

As memory space on microcontrollers is often at a premium, each of the recipe
commands are encoded into one byte. These recipes are typically implemented as an
array of byte values.

A master timer generates interrupts every 100 milliseconds, serving as the time basis
the wait commands below.

Command Format: Opcode(3 bits) - Command Parameter(5 bits)

Recipe Commands:

| Mnemonic   | Opcode | Parameter                                                                            | Range | Comments                                                                                    |
|------------|--------|--------------------------------------------------------------------------------------|-------|---------------------------------------------------------------------------------------------|
| MOV        | 001    | The target position number                                                           | 0..5  | An out of range parameter value produces an error. *                                        |
| WAIT       | 010    | The number of 1/10 seconds to delay before attempting to execute next recipe command | 0..31 | The actual delay will be 1/10 second more than the parameter value.. **                     |
| LOOP_START | 100    | The number of additional times to execute the following recipe block                 | 0..31 | A parameter value of “n” will execute the block once but will repeat it “n” more times. *** |
| END_LOOP   | 101    | NA                                                                                   | NA    | Marks the end of a recipe loop block.                                                       |
| RECIPE_END | 000    | NA                                                                                   | NA    |                                                                                             |


