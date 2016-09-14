#include "stm32l476xx.h"
#include "SysClock.h"
#include "LED.h"
#include "UART.h"

#include <string.h>
#include <stdio.h>

#define NUM_BUCKETS 101 	     /* one bucket for each millisecond */
#define MAX_MEASUREMENTS 1000        /* number of measurements to make */
#define ONE_HUNDRED_THOUSAND 100000  /* macro for value 100000 */
#define MAX_INPUT_DIGITS 4           /* largest digit user may enter for limit */
#define NEWLINE_IDENTIFIER 13	     /* ascii value for a newline */

/* Global Variables */
char repeat_prompt[] = "Would you like to generate another histogram? (Y = yes, N = no):\r\n";
char post_prompt[] = "Would you like to try another power on self test? (Y = yes, N = no):\r\n";

uint8_t buffer[100];           /* char array to hold anything to output */
uint16_t results[NUM_BUCKETS]; /* array to hold our 1000 measurements */
uint16_t low_limit = 950;      /* default lower limit of 950 microseconds */
uint32_t initial_time = 0;     /* global time variable for ease of POST routine */

/**
    Description: This function is responsible for handling a failed POST routine. If
		 POST fails, then we ask the user if they want to try another test, otherwise
		 we end the program.
 **/
void POST_failed(){

	char rxByte;

	USART_Write(USART2, (uint8_t *)"Power On Self Test FAILED\r\n", 29);

        // ask user if they want to try POST again
	USART_Write(USART2, (uint8_t *)post_prompt, strlen(post_prompt));
	rxByte = USART_Read(USART2);

	//process user input
	if (rxByte == 'N' || rxByte == 'n'){
		
		Red_LED_On();
		USART_Write(USART2, (uint8_t *)"Exiting Program\r\n", 20);
		while(1);

	} else if(rxByte == 'Y' || rxByte == 'y'){
		
		initial_time = (uint32_t)TIM2->CCR1;
	}

}

/**
    Description: This function is responsible for completing the power on self test
	         routine. In this project, our routine simply checks if our pulse generator
		 is connected and that were recieving 5v singals at a 10 KHz rate.
 **/
void POST(){

	uint8_t test_complete = 0;
	uint32_t curr_time = 0;

	USART_Write(USART2, (uint8_t *)"Beginning Power On Self Test...\r\n\r\n", 37);

	TIM2->CR1 = 0x1; 						// begin input capture
	initial_time = (uint32_t)TIM2->CCR1;	// grab inital time

	while(!test_complete){

		// if we have a reading
		if(TIM2->SR & 0x2){

			curr_time = (uint32_t)TIM2->CCR1;
			
			// if the reading within 100 ms, test pass
			if(curr_time - initial_time <= ONE_HUNDRED_THOUSAND){
				
				test_complete = 1;
				USART_Write(USART2, (uint8_t *)"Power On Self Test SUCCESS\r\n\r\n", 32);
				break;
			} else {
				
				// otherwise POST failed
				POST_failed();	
			}
			
		} else {

			curr_time = (uint32_t)TIM2->CNT;

			// check if we've timed out
			if(curr_time - initial_time > ONE_HUNDRED_THOUSAND){
				POST_failed();
			}
		}
	}

	// stop input capture
	TIM2->CR1 = 0x0;
}

/**
    Description: This funciton is responsible for initializing our timer. We will use pin PA0 in
	         alternate function 1 (TIM2_CH1) in order to get timer functionality. We will also
		 configure the timer to recieve input capture on that pin.
 **/
void Timer_init(){

	// enable the peripheral clock of GPIO Port
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

	// initialize PA0 as Timer 2, Channel 1
	GPIOA->MODER &= ~(0xFFFFFFFF); // clear bits
	GPIOA->MODER |= 0x2;	       // alternate function mode for PA0
	GPIOA->AFR[0] |= 0x1;	       // PA0 alternate function 1 (TIM2_CH1)

	// enable our timer
	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;

	// configure timer
	TIM2->PSC = 80; 	     // load prescale value (1MHz)
	TIM2->EGR |= TIM_EGR_UG;     // force load of new prescaler
	TIM2->CCER &= ~(0xFFFFFFFF); // turn off capture input until we're ready with updates
	TIM2->CCMR1 |= 0x1;	     // set compare/capture channel for input and clear filter
	TIM2->CCER |= 0x1;	     // enable capture input
}

/**
    Description: This function is responsible for actually reading the data. To accomplish this,
		 it will turn on input caputre, and store the time difference between 1000 different
		 rising edges from our pulse generator.
 **/
void Measure_pulses(){

	uint8_t got_reading = 0;
	uint32_t last_reading, curr_time, difference, count = 0;

	TIM2->CR1 = 0x1; // begin input capture

	while(1){

		// check if input has been captured
		if(TIM2->SR & 0x2){

			// break out once we've finished measurements
			if(count >= MAX_MEASUREMENTS) { 
				TIM2->CR1 = 0x0;
				break; 
			}

			// catch first reading for accurate first period
			if(!got_reading){

				last_reading = (uint32_t)TIM2->CCR1;
				got_reading = 1;
			} else {

				curr_time = (uint32_t)TIM2->CCR1;
				difference = curr_time - last_reading;
				last_reading = curr_time;
				
				// if the reading is between our bounds
				if(difference >= low_limit && difference <= low_limit+100){
				
					results[difference - low_limit]++;
					count++;
				}
			}
		}
	}
}

/**
    Description: This function is responsible for displaying the measured data.
 **/
void Build_histogram(){

	int initial_val = low_limit - 1;

	USART_Write(USART2, (uint8_t *)"\r\nHISTORGRAM DATA:\r\n", 22);

	// for each time result
	for(int i = 0; i < NUM_BUCKETS; i++){

		initial_val++;

		// if there were results for this time
		if(results[i] != 0){

			// print value with num occurances
			memset( buffer, '\0', sizeof(buffer));
			sprintf((char *)buffer, "%d microseconds --> %d occurance(s)\r\n", initial_val, results[i]);
			USART_Write(USART2, buffer, sizeof(buffer));
		}

	}

	USART_Write(USART2, (uint8_t *)"\r\n", 4);
}

/**
    Description: This function is responsible for handling the user input for the new lower
	         limit. This function will read each character and echo it to the serial comm
		 while the user hasn't hit enter, and hasn't gone over the 4 digit limit.
		 Note: Good user input is expected
 **/
void Get_lower_bound(){

	int i = 0;
	char rxByte;
	char inputBuffer[MAX_INPUT_DIGITS+1] = {'\0', '\0', '\0', '\0', '\0'};

	USART_Write(USART2, (uint8_t *)"Enter your new lower bound then hit enter (50-9950):\r\n", 57);
	rxByte = USART_Read(USART2);

	// while the user hasn't hit enter
	while((rxByte != NEWLINE_IDENTIFIER) && i<MAX_INPUT_DIGITS){

		memset( buffer, '\0', sizeof(buffer));
		sprintf((char *)buffer, "%c", rxByte);
		USART_Write(USART2, buffer, sizeof(buffer));

		inputBuffer[i] = rxByte;
		i++;

		rxByte = USART_Read(USART2);
	}

	sscanf(inputBuffer, "%d", &low_limit);
	sprintf((char *)buffer, "New lower limit: %d\r\n", low_limit);
	USART_Write(USART2, buffer, sizeof(buffer));

}

/**
    Description: This function is responsible for prompting the user for a new lower bound
	         value. This lower bound value between 50 and 9950 microseconds represents
		 the lower bound of the period we are trying to capture. Get_lower_bound()
		 is the helper function to handles the input.
 **/
void Configure_bounds(){

	char rxByte;

        // ask user if he/she wants to update lower bound
	sprintf((char *)buffer, "Would you like to change the current lower bound of %d microseconds? (Y = yes, N = no):\r\n", low_limit);
	USART_Write(USART2, buffer, sizeof(buffer));

	rxByte = USART_Read(USART2);

	// if yes, get lower bound
	if (rxByte == 'Y' || rxByte == 'y'){

		// get user input
		Get_lower_bound();
	}
}

int main(void){

    char rxByte;

	/* initialization routine */
	System_Clock_Init(); // Switch System Clock = 80 MHz
	LED_Init();
	UART2_Init();
	Timer_init();

        // Power On Self Test
	POST();

	while(1){

		// prompt user for new bounds
		Configure_bounds();

		// perform measurements
		Measure_pulses();

		// output historgram
		Build_histogram();

		// ask user if they want to go again
		USART_Write(USART2, (uint8_t *)repeat_prompt, strlen(repeat_prompt));

		rxByte = USART_Read(USART2);

		// if no; break out of loop
		if (rxByte == 'N' || rxByte == 'n'){
			Red_LED_On();
			USART_Write(USART2, (uint8_t *)"Exiting Program\r\n", 20);
			break;
		}

		// clear past measurements
		for(int i = 0; i < NUM_BUCKETS; i++){
			results[i] = 0;
		}
	}
}
