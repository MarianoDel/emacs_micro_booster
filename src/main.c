/**
  ******************************************************************************
  * @file    Template_2/main.c
  * @author  Nahuel
  * @version V1.0
  * @date    22-August-2014
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * Use this template for new projects with stm32f0xx family.
  *
  ******************************************************************************
  */


/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "gpio.h"
#include "tim.h"
#include "uart.h"
#include "hard.h"

#include "core_cm0.h"
#include "adc.h"
#include "flash_program.h"

#include "stm32f0xx_it.h"



//--- VARIABLES EXTERNAS ---//


// ------- Externals del Puerto serie  -------
volatile unsigned char tx1buff[SIZEOF_DATA];
volatile unsigned char rx1buff[SIZEOF_DATA];
volatile unsigned char usart1_have_data = 0;

// ------- Externals del o para el ADC -------
#ifdef ADC_WITH_INT

volatile unsigned short adc_ch[4];

#define Iout_Sense	adc_ch[0]
#define Vin_Sense	adc_ch[1]
#define I_Sense		adc_ch[2]
#define Vout_Sense	adc_ch[3]

volatile unsigned char seq_ready = 0;
unsigned short zero_current;

#endif

// ------- Externals para filtros -------
unsigned short mains_voltage_filtered;
//
//
// volatile unsigned short scroll1_timer = 0;
// volatile unsigned short scroll2_timer = 0;
//
// volatile unsigned short standalone_timer;
// volatile unsigned short standalone_enable_menu_timer;
// //volatile unsigned short standalone_menu_timer;
// volatile unsigned char grouped_master_timeout_timer;
volatile unsigned short take_temp_sample = 0;


parameters_typedef param_struct;

//--- VARIABLES GLOBALES ---//






// ------- de los timers -------
volatile unsigned short wait_ms_var = 0;
volatile unsigned short timer_standby;
//volatile unsigned char display_timer;
volatile unsigned char timer_meas;

volatile unsigned char door_filter;
volatile unsigned char take_sample;
volatile unsigned char move_relay;


volatile unsigned short secs = 0;
volatile unsigned char hours = 0;
volatile unsigned char minutes = 0;






//--- FUNCIONES DEL MODULO ---//
void TimingDelay_Decrement(void);

// ------- del DMX -------
// extern void EXTI4_15_IRQHandler(void);


//--- FILTROS DE SENSORES ---//
#define LARGO_FILTRO 16
#define DIVISOR      4   //2 elevado al divisor = largo filtro
//#define LARGO_FILTRO 32
//#define DIVISOR      5   //2 elevado al divisor = largo filtro
unsigned short vtemp [LARGO_FILTRO + 1];
unsigned short vpote [LARGO_FILTRO + 1];

//--- FIN DEFINICIONES DE FILTRO ---//


//--- Private Definitions ---//


//-------------------------------------------//
// @brief  Main program.
// @param  None
// @retval None
//------------------------------------------//
int main(void)
{
	unsigned char i, ii;

	unsigned char need_to_save = 0;
	unsigned short power, last_power;
	unsigned int zero_current_loc = 0;

	main_state_t main_state = MAIN_INIT;


	unsigned short hyst;

	char s_lcd [100];		//lo agrando porque lo uso tambien para enviar SMS

	//GPIO Configuration.
	GPIO_Config();

	//ACTIVAR SYSTICK TIMER
	if (SysTick_Config(48000))
	{
		while (1)	/* Capture error */
		{
			if (LED)
				LED_OFF;
			else
				LED_ON;

			for (i = 0; i < 255; i++)
			{
				asm (	"nop \n\t"
						"nop \n\t"
						"nop \n\t" );
			}
		}
	}

	//--- Leo los parametros de memoria ---//
	LED_OFF;
	while (1);

	while (1)
	{
		// if (STOP_JUMPER)
		// {
		// 	LED_OFF;
		// }
		// else
		// {
			// if (LED)
			// 	LED_OFF;
			// else
			// 	LED_ON;

			LED_ON;
			Wait_ms (250);
			LED_OFF;
			Wait_ms (250);
		// }
	}


	//--- Welcome code ---//
	LED_OFF;

	USART1Config();
	EXTIOff();



//---------- Pruebas con GSM GATEWAY --------//
// AdcConfig();		//recordar habilitar sensor en adc.h

TIM_3_Init ();					//lo utilizo para 1 a 10V y para synchro ADC

TIM_16_Init();					//o utilizo para synchro de relay
TIM16Enable();

Usart1Send((char *) (const char *) "\r\nKirno Placa Redonda - Basic V1.0\r\n");
Usart1Send((char *) (const char *) "  Features:\r\n");
#ifdef WITH_1_TO_10_VOLTS
Usart1Send((char *) (const char *) "  Dimmer 1 to 10V\r\n");
#endif
#ifdef WITH_HYST
Usart1Send((char *) (const char *) "  Night Hysteresis\r\n");
#endif
#ifdef WITH_TEMP_CONTROL
Usart1Send((char *) (const char *) "  Temp Control\r\n");
#endif
#ifdef USE_WITH_SYNC
Usart1Send((char *) (const char *) "  Sync by Edges\r\n");
#else
Usart1Send((char *) (const char *) "  Sync by ADC\r\n");
#endif
#ifdef USE_GSM
Usart1Send((char *) (const char *) "  Uses GSM for SMS data\r\n");
#endif

while (1)
{
	Update_TIM3_CH1 (10);		//lo uso para ver diff entre synchro adc con led
	main_state = SYNCHRO_ADC;
	ADC1->CR |= ADC_CR_ADSTART;
	seq_ready = 0;

}




//---------- Inicio Programa de Produccion Redonda Basic --------//
//--- FIN Programa de pruebas synchro de Relay -----
//--- Programa de Redonda Basic - Produccion - -----

//---------- Fin Programa de Produccion Redonda Basic--------//


	return 0;
}

//--- End of Main ---//





void TimingDelay_Decrement(void)
{
	if (wait_ms_var)
		wait_ms_var--;

	if (timer_standby)
		timer_standby--;


	if (take_temp_sample)
		take_temp_sample--;

	if (timer_meas)
		timer_meas--;

	// //cuenta de a 1 minuto
	// if (secs > 59999)	//pasaron 1 min
	// {
	// 	minutes++;
	// 	secs = 0;
	// }
	// else
	// 	secs++;
	//
	// if (minutes > 60)
	// {
	// 	hours++;
	// 	minutes = 0;
	// }


}

//------ EOF -------//