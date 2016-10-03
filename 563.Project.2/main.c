#include "stm32l476xx.h"
#include "SysClock.h"
#include "LED.h"
#include "UART.h"
#include "servo.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define TWO_HUNDRED (200)
#define MAX_INPUT_DIGITS (3)           /* largest digit user may enter for limit */
#define NEWLINE_IDENTIFIER (13)		   /* ascii value for a newline */
#define BACKSPACE_IDENTIFIER (127)	   /* ascii value for a backspace */

char begin_prompt[] = "Enter 'Cc' to begin routine:\r\n>";

servo_data motor_data[NUM_MOTORS]; /* holds data for both motors (see servo.h) */

/**
    Description: This funciton is responsible for initializing our timer. We will use pin PA0 in
	             alternate function 1 (TIM2_CH1) and PA1 in alternate function 1 (TIM2_CH2) in order 
				 to get timer functionality. We will configure both timer channels as output and put them
				 both into PWM mode.
 **/
void Timer_init(){

	// enable the peripheral clock of GPIO Port
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

	// initialize PA0 as Timer 2, Channel 1 and PA1 as Timer 2, Channel 2
	GPIOA->MODER &= ~(0xFFFFFFFF); // clear bits
	GPIOA->MODER |= 0xA;		   // alternate function mode for PA0 and PA1
	GPIOA->AFR[0] |= 0x11;		   // PA0 AF 1 (TIM2_CH1), PA1 AF 1 (TIM2_CH2)

	// enable our timer
	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;

	// configure timer
	TIM2->PSC = 8000;
	
	TIM2->CCMR1 &= ~(0x00000303); // configure channels 1 and 2 as output
	TIM2->CCMR1 |= (0x00006868); // enable output compare preload and set modes for both channels
	
	TIM2->CR1 |= (0x00000080);  // enable auto-reload of preload
	TIM2->CCER |= (0x00000011); // enable channels as output
	
	TIM2->ARR = 200;
	
	// initialize motor positions to 0 degrees
	TIM2->CCR1 = pos_pulse_widths[POS_0];
	TIM2->CCR2 = pos_pulse_widths[POS_0];
	
	TIM2->EGR |= TIM_EGR_UG;  // force load of new values
	
	TIM2->CR1 = 0x1;
	
}

/**
    Description: This funciton is a simple delaying function. It will simply cycle in a while loop for the
				 desired time in microseconds.
	Input: unsigned int us: number of microseconds to delay
 **/
void delay(uint32_t us) {
	
	if(us > 0){
		uint32_t time = 100*us/7;    
		while(--time);   
	}
}

/**
    Description: This funciton is responsible for modifying the width of the pulse modulation for each of the timer
				 channels. This mimics that actual movement of the motor positions.
	Input: unsigned 8 bit motor: the motor id (0 or 1)
		   unsigned 16 bit goto_pos: the position to move the motor to (0-5)
 **/
void move_motor(uint8_t motor, uint16_t goto_pos){

	if(motor == 0){
		TIM2->CCR1 = pos_pulse_widths[goto_pos];
		motor_data[0].pos = (position)goto_pos;
	}
	
	if(motor == 1){ 
		TIM2->CCR2 = pos_pulse_widths[goto_pos]; 
		motor_data[1].pos = (position)goto_pos;
	}
}

/**
    Description: This funciton is responsible for processing the user input at runtime.
	Input: char input[3]: the two character user input for each motor.
 **/
void process_input(char input[MAX_INPUT_DIGITS+1]){

	uint8_t has_moved = 0;
	
	// for each motor
	for(int i = 0; i < NUM_MOTORS; i++){

		switch(input[i]){

			case 'P':
			case 'p':
				motor_data[i].status = PAUSED;
				break;
			case 'C':
			case 'c':
				motor_data[i].status = RUNNING;
				break;
			case 'R':
			case 'r':
				if(motor_data[i].pos != POS_5){ 
					move_motor(i, motor_data[i].pos+1);
					has_moved = 1;
				}
				break;
			case 'L':
			case 'l':
				if(motor_data[i].pos != POS_0){
					move_motor(i, motor_data[i].pos-1);
					has_moved = 1;
				}
				break;
			case 'N':
			case 'n':
				break;
			case 'B':
			case 'b':
				motor_data[i].instruction_index = 0;
			  motor_data[i].status = RUNNING;
				break;
			default:
				// invalid input
				USART_Write(USART2, (uint8_t *)"\r\nInvalid Command\r\n>", 18);
		}

	}
	
	/* delay 200 ms for 1 motor position move */
	if(has_moved){ delay(200000);}
}

/**
    Description: This funciton is responsible reading the user input at runtime. If the UART detects
				 a character, the program will block until the user finishes his/her input.
 **/
void get_input(){

	char rxByte = '\0';
	char inputBuffer[MAX_INPUT_DIGITS+1] = {'\0', '\0', '\0', '\0'};
	uint8_t index = 0;

	rxByte = USART_Read_First(USART2);

	/* if we've read a characgter */
	if(rxByte != '\0'){

		/* while the user doesn't hit enter */
		while(rxByte != NEWLINE_IDENTIFIER){

			/* if the user enters an 'x', stop reading */
			if(rxByte == 'x' || rxByte == 'X'){ 
				USART_Write(USART2, (uint8_t*)&rxByte, sizeof(char));
				USART_Write(USART2, (uint8_t *)"\r\n>", 4);
				return;
			};

			/* handle backspace logic */
			if(rxByte == BACKSPACE_IDENTIFIER){

				if(index > 0){
					index--;
					USART_Write(USART2, (uint8_t*)&rxByte, sizeof(char));
				}

			/* otherwise store character */
			} else {

				if(index >= MAX_INPUT_DIGITS-1){ break; }

				inputBuffer[index] = rxByte;
				index++;
				USART_Write(USART2, (uint8_t*)&rxByte, sizeof(char));
			}

			/* UART read that blocks */
			rxByte = USART_Read(USART2);
		}

		/* process the input when done */
		process_input( inputBuffer );
		USART_Write(USART2, (uint8_t *)"\r\n>", 4);
	}
}

/**
    Description: This funciton is responsible for processing the current recipe command for each
				 of the two servo motors.
 **/
void process_recipe(){
	
	/* potential moves made for delay purposes */
	uint8_t pos_difference = 0;
	uint8_t wait = 0;
	uint8_t recipe_ended = 0;
	
	/* for each motor */
	for(int i = 0; i < NUM_MOTORS; i++){
		
		/* if the motor is in a RUNNING state */
		if(motor_data[i].status == RUNNING){
			
			/* switch on opcodes */
			switch(recipe[i][motor_data[i].instruction_index] & OPCODE_MASK){
				
				case MOV:
					
					if((recipe[i][motor_data[i].instruction_index] & PARAMETER_MASK) > POS_5){ 
					
						/* fail state must restart */
						Red_LED_On();
						
						USART_Write(USART2, (uint8_t *)"Error State; Check recipe\r\n>", 28);
						
						while(1);
						
					}
					else{
						
						/* record the largest position move */
						if(abs(motor_data[i].pos - (recipe[i][motor_data[i].instruction_index] & PARAMETER_MASK)) > pos_difference){
							pos_difference = abs(motor_data[i].pos - (recipe[i][motor_data[i].instruction_index] & PARAMETER_MASK));
						}
						
						/* move motor */
						move_motor(i, (recipe[i][motor_data[i].instruction_index] & PARAMETER_MASK));
					}
					motor_data[i].instruction_index++;
					break;
				case WAIT:
				
					/* record the longest wait time between the two possible recipe commands */
					if((recipe[i][motor_data[i].instruction_index] & PARAMETER_MASK) > wait){
						wait = (recipe[i][motor_data[i].instruction_index] & PARAMETER_MASK);
					}
					
					motor_data[i].instruction_index++;
				
					break;
				case LOOP_START:
					
					/* if were already in a loop (no nested loops) */
					if(motor_data[i].in_loop == 1){ 
					
						/* fail state must restart */
						Red_LED_On();
						
						USART_Write(USART2, (uint8_t *)"Error State; Check recipe\r\n>", 28);
						
						while(1);
					
					}
					else{
						/* handle looping logic */
						motor_data[i].in_loop = 1;
						motor_data[i].loop_index = motor_data[i].instruction_index + 1;
						motor_data[i].loop_count = (recipe[i][motor_data[i].instruction_index] & PARAMETER_MASK) - 1;
						motor_data[i].instruction_index++;
					}
					
					break;
				case END_LOOP:
					
					/* if were not in a loop */
					if(motor_data[i].in_loop != 1){ 
					
						/* fail state must restart */
						Red_LED_On();
						
						USART_Write(USART2, (uint8_t *)"Error State; Check recipe\r\n>", 28);
						
						while(1);
						
					} else{
						
						// done with loop
						if(motor_data[i].loop_count < 0){
							
							motor_data[i].in_loop = 0;
							motor_data[i].instruction_index ++;
							
						// move back to first instruction in loop	
						} else {
							
							motor_data[i].instruction_index = motor_data[i].loop_index;
							motor_data[i].loop_count--;
							
						}
					}
				
					break;
				case RECIPE_END:
					motor_data[i].instruction_index = 0;
					motor_data[i].status = PAUSED;
					recipe_ended = 1;
				
					break;
				default:
					// invalid command
					USART_Write(USART2, (uint8_t *)"Invalid Recipe\r\n>", 17);
					break;
			}
			
		}
	} 
	
	/* delay the max move difference */
	if(pos_difference){ delay(200000 * pos_difference); }
	
	/* wait the max wait command length */
	if(wait){ delay(100000 * wait); }
	
	/* tell user if recipe has ended */
	if(recipe_ended) { USART_Write(USART2, (uint8_t *)"Recipe ended; 'cc' to replay\r\n>", 31); }
}

int main(void){

	/* initialization routine */
	System_Clock_Init(); // Switch System Clock = 80 MHz
	LED_Init();
	UART2_Init();
	Timer_init();

	USART_Write(USART2, (uint8_t *)begin_prompt, strlen(begin_prompt));

	/* constantly check user input and then process current recipe commands */
	while(1){

			get_input();
		
			Green_LED_On();
		
			process_recipe();
		
			Green_LED_Off();
		
	};
}
