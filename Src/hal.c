/*
 * Copyright (c) 2014-2016 IBM Corporation.
 * All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of the <organization> nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This file is a futher development from the great IBM LMIC
 * made by Hjalmar SkovHolm Hansen
 * Denmark
 */

#include "lmic.h"
#include "main.h"

#define myTIMER htim4   //  <--------- change to your setup
#define mySPI hspi1     //  <--------- change to your setup

/*  ************************************** */
/*    DO NOT CHANGE BELOW THIS LINE        */
/*  ************************************** */


// HAL state
static struct {
	int irqlevel;
    u4_t ticks;
} HAL;

// -----------------------------------------------------------------------------
// I/O

static void hal_io_init () {
	//already done by cubemx
}

// val ==1  => tx 1, rx 0 ; val == 0 => tx 0, rx 1
void hal_pin_rxtx (u1_t val) {
#ifdef RX_GPIO_Port
  #ifdef TX_GPIO_Port
    HAL_GPIO_WritePin(RX_GPIO_Port,RX_Pin,~val);
    HAL_GPIO_WritePin(TX_GPIO_Port,TX_Pin,val);
  #endif
#endif
}

// set radio NSS pin to given value
void hal_pin_nss (u1_t val) {
    HAL_GPIO_WritePin(NSS_GPIO_Port,NSS_Pin,val);
}

// set radio RST pin to given value (or keep floating!)
void hal_pin_rst (u1_t val) {
    if(val == 0 || val == 1) { // drive pin
    	GPIO_InitTypeDef GPIO_InitStruct;
    	GPIO_InitStruct.Pin = RESET_Pin;
    	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    	HAL_GPIO_Init(RESET_GPIO_Port, &GPIO_InitStruct);

    	HAL_GPIO_WritePin(RESET_GPIO_Port,RESET_Pin,val);

    } else { // keep pin floating
        GPIO_InitTypeDef GPIO_InitStruct;
    	GPIO_InitStruct.Pin = RESET_Pin;
    	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    	GPIO_InitStruct.Pull = GPIO_NOPULL;
    	HAL_GPIO_Init(RESET_GPIO_Port, &GPIO_InitStruct);
    }
}

extern void radio_irq_handler(u1_t dio);

// generic EXTI IRQ handler for all channels
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin_int){
	// DIO 0
	if(GPIO_Pin_int == DIO0_Pin) {
		// invoke radio handler (on IRQ!)
		radio_irq_handler(0);
	}
	// DIO 1
	if(GPIO_Pin_int == DIO1_Pin) {
	    // invoke radio handler (on IRQ!)
		radio_irq_handler(1);
	}
	// DIO 2
	if(GPIO_Pin_int == DIO2_Pin) {
	    // invoke radio handler (on IRQ!)
	    radio_irq_handler(2);
	}
}

// -----------------------------------------------------------------------------
// SPI
void hal_spi_init () {
	// already done by cube mx
}

// perform SPI transaction with radio
u1_t hal_spi (u1_t out) {
	char outbuffer[] ="";
	char inbuffer[] ="";
	outbuffer[0] = out;
	HAL_SPI_TransmitReceive(&mySPI,outbuffer,inbuffer,sizeof(outbuffer),HAL_MAX_DELAY);
	return inbuffer[0];
}


// -----------------------------------------------------------------------------
// TIME
static void hal_time_init () {
	// already done by cubemx
}

u4_t hal_ticks () {
    hal_disableIRQs();
    u4_t t = HAL.ticks;
    u2_t cnt = __HAL_TIM_GET_COUNTER(&myTIMER);
    if(__HAL_TIM_GET_FLAG(&myTIMER, TIM_FLAG_CC1) != RESET){
    	if(__HAL_TIM_GET_IT_SOURCE(&myTIMER, TIM_IT_CC1) !=RESET){
    		cnt = __HAL_TIM_GET_COUNTER(&myTIMER);
    		t++;
        }
     }
    hal_enableIRQs();
    return (t<<16)|cnt;
}

// return modified delta ticks from now to specified ticktime (0 for past, FFFF for far future)
static u2_t deltaticks (u4_t time) {
    u4_t t = hal_ticks();
    s4_t d = time - t;
    if( d<=0 ) return 0;    // in the past
    if( (d>>16)!=0 ) return 0xFFFF; // far ahead
    return (u2_t)d;
}

void hal_waitUntil (u4_t time) {
    while( deltaticks(time) != 0 ); // busy wait until timestamp is reached
}

// check and rewind for target time
u1_t hal_checkTimer (u4_t time) {
    u2_t dt;
    myTIMER.Instance->SR &= ~TIM_SR_CC1IF; // clear any pending interrupts
    if((dt = deltaticks(time)) < 5) { // event is now (a few ticks ahead)
    	myTIMER.Instance->DIER &= ~TIM_DIER_CC1IE; // disable IE
        return 1;
    } else { // rewind timer (fully or to exact time))
    	myTIMER.Instance->CCR1 = myTIMER.Instance->CNT + dt;   // set comparator
    	myTIMER.Instance->DIER |= TIM_DIER_CC1IE;  // enable IE
    	myTIMER.Instance->CCER |= TIM_CCER_CC1E;   // enable capture/compare uint 2
        return 0;
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if(htim->Instance == myTIMER.Instance){
		HAL.ticks++;
    }
}

// -----------------------------------------------------------------------------
// IRQ
void hal_disableIRQs () {
	__disable_irq();
	//__set_BASEPRI(1 << 4);
    HAL.irqlevel++;
}

void hal_enableIRQs () {
    if(--HAL.irqlevel == 0) {
		__enable_irq();
    	//__set_BASEPRI(0);
    }
}

void hal_sleep () {
	// low power sleep mode
#ifndef CFG_no_low_power_sleep_mode
	// PWR->CR |= PWR_CR_LPSDSR;
#endif
    // suspend execution until IRQ, regardless of the CPSR I-bit
    __WFI();
}

// -----------------------------------------------------------------------------

void hal_init () {
    memset(&HAL, 0x00, sizeof(HAL));
    hal_disableIRQs();
    // configure radio I/O and interrupt handler
    hal_io_init();
    // configure radio SPI
    hal_spi_init();
    // configure timer and interrupt handler
    hal_time_init();
    hal_enableIRQs();
}

void hal_failed () {
    // HALT...
    hal_disableIRQs();
    hal_sleep();
    while(1);
}

