//-----------------------------------------------------
// #### PROYECTO MICROINVERSOR F030 - Custom Board ####
// ##
// ## @Author: Med
// ## @Editor: Emacs - ggtags
// ## @TAGS:   Global
// ## @CPU:    STM32F030
// ##
// #### MAIN.C ########################################
//-----------------------------------------------------

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
#include "dsp.h"

#include "it.h"



//--- VARIABLES EXTERNAS ---//


// ------- Externals del Puerto serie  -------
volatile unsigned char tx1buff[SIZEOF_DATA];
volatile unsigned char rx1buff[SIZEOF_DATA];
volatile unsigned char usart1_have_data = 0;

// ------- Externals del o para el ADC -------
#ifdef ADC_WITH_INT

volatile unsigned short adc_ch[ADC_CHANNEL_QUANTITY];
volatile unsigned char seq_ready = 0;


#endif

// ------- Externals para timers -------
volatile unsigned short timer_led = 0;


// ------- Externals para filtros -------
volatile unsigned short take_temp_sample = 0;

// ------- Definiciones para los filtros -------
#define SIZEOF_FILTER    8
#define UNDERSAMPLING_TICKS    5
unsigned short vin_vector [SIZEOF_FILTER];
// unsigned short vbatt [SIZEOF_FILTER];
// unsigned short iboost [SIZEOF_FILTER];


// parameters_typedef param_struct;

//--- VARIABLES GLOBALES ---//
volatile unsigned char current_excess = 0;
short d = 0;
short ez1 = 0;
short ez2 = 0;
unsigned short dmax = 0;


// ------- de los timers -------
volatile unsigned short wait_ms_var = 0;
volatile unsigned short timer_standby;
//volatile unsigned char display_timer;
volatile unsigned char timer_meas;
volatile unsigned char timer_filters = 0;

// volatile unsigned short secs = 0;
// volatile unsigned char hours = 0;
// volatile unsigned char minutes = 0;






//--- FUNCIONES DEL MODULO ---//
void TimingDelay_Decrement (void);
void Overcurrent_Shutdown (void);

#ifdef VER_1_0
// ------- para el LM311 -------
extern void EXTI0_1_IRQHandler(void);
#endif

#ifdef VER_1_1
// ------- para el LM311 -------
extern void EXTI4_15_IRQHandler(void);
#endif



//--- Private Definitions ---//


//-------------------------------------------//
// @brief  Main program.
// @param  None
// @retval None
//------------------------------------------//
int main(void)
{
    unsigned char i, ii;

    unsigned char undersampling = 0;
    main_state_t main_state = MAIN_INIT;
    unsigned short vin_filtered = 0;

    char s_lcd [100];		

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

    // hile (1)
    // {
    //  if (STOP_JUMPER)
    //  {
    //  	LED_OFF;
    //  }
    //  else
    //  {
    // 	  if (LED)
    // 	  	LED_OFF;
    // 	  else
    // 	  	LED_ON;
    //
    // 	  Wait_ms (250);
    //  }
    // }


//---------- Pruebas de Hardware --------//

//---------- Test INT VER_1_1 --------//
#ifdef TEST_INT_PRGRM
    //arranca como programa de produccion pero no mueve led, solo lo prende en INT
    //RECORDAR QUITAR JUMPER en driver (para no mover mosfets)
    //colocar generador de funciones en I_MOS_A o I_MOS_B senial triangular
    EXTIOff ();    
    USART1Config();
    AdcConfig();		//recordar habilitar sensor en adc.h

    TIM_1_Init ();					//lo utilizo para mosfet Ctrol_M_B,
    TIM_3_Init ();					//lo utilizo para mosfet Ctrol_M_A y para synchro ADC

    while (1)
    {
        switch (main_state)
        {
        case MAIN_INIT:
            main_state = MAIN_SYNCHRO_ADC;
            ADC1->CR |= ADC_CR_ADSTART;
            seq_ready = 0;
            break;

        case MAIN_SYNCHRO_ADC:
            if (seq_ready)
            {
                Usart1Send((char *) (const char *) "ADC Sync getted!\r\n");
                main_state = MAIN_SET_ZERO_CURRENT;
                seq_ready = 0;
                timer_standby = 2000;
            }
            break;

        case MAIN_SET_ZERO_CURRENT:
            if (!timer_standby)
            {
                //a esta altura debo tener bien medida la tension de alimentacion para poder determina dmax
                d = 0;
                main_state = MAIN_GENERATING;
                EXTIOn();
            }                
            break;

        case MAIN_GENERATING:
            if (!STOP_JUMPER)
            {
                if (seq_ready)
                {
                    seq_ready = 0;

                    if (undersampling < UNDERSAMPLING_TICKS)
                        undersampling++;
                    else
                    {
                        d = PID_roof (VOUT_200V, Vout_Sense, d, &ez1, &ez2);
                    
                        if (d < 0)
                            d = 0;

                        if (d > dmax)
                            d = dmax;

                        UpdateTIMSync (d);
                        dmax = UpdateDMAX(vin_filtered);    //TODO: luego meter el filtro en sync con mustras
                    }
                }    //cierra sequence
            }
            else
            {
                UpdateTIMSync (0);
                d = 0;
                timer_standby = 300;    //doy minimo 300ms para reactivar
                main_state = MAIN_JUMPER_PROTECTED;
            }            
            break;

        case MAIN_JUMPER_PROTECTED:
            if (!timer_standby)
            {
                if (!STOP_JUMPER)
                {
                    //vuelvo a INIT
                    main_state = MAIN_SET_ZERO_CURRENT;
                }
            }                
            break;

        case MAIN_OVERCURRENT:
            if ((!PROT_MOS_A) && (!PROT_MOS_B))
            {
                LED_OFF;
                ENABLE_TIM3;
                ENABLE_TIM1;
                seq_ready = 0;
                // main_state = MAIN_SYNCHRO_ADC;
                main_state = MAIN_GENERATING;
            }
            break;

        default:
            seq_ready = 0;
            main_state = MAIN_SYNCHRO_ADC;
            break;
        }	//fin switch main_state

        if (!timer_standby)
        {
            timer_standby = 2000;
            sprintf (s_lcd, "VIN: %d, VOUT: %d, d: %d\r\n", Vin_Sense, Vout_Sense, d);
            Usart1Send(s_lcd);
        }

        if (current_excess)
        {
            current_excess = 0;
            Usart1Send("\r\n Overcurrent!");
            main_state = MAIN_OVERCURRENT;
        }

        if (!timer_filters)
        {
            //espero un poco entre cada muestra de la tension
            timer_filters = 3;
            vin_vector[0] = Vin_Sense;
            vin_filtered = MAFilter8(vin_vector);
        }
            
            

        UpdateLed();
        
    }	//fin while 1
#endif TEST_INT_PRGRM


//---------- Programa de Produccion --------//    
    //--- Welcome code ---//
#ifdef PRODUCTION_PRGRM
    EXTIOff ();
    USART1Config();
    AdcConfig();		//recordar habilitar sensor en adc.h

    TIM_1_Init ();					//lo utilizo para mosfet Ctrol_M_B,
    TIM_3_Init ();					//lo utilizo para mosfet Ctrol_M_A y para synchro ADC
#ifdef VER_1_0
    TIM_14_Init();					//Set current overflow
#endif

    while (1)
    {
        switch (main_state)
        {
        case MAIN_INIT:
            main_state = MAIN_SYNCHRO_ADC;
            ADC1->CR |= ADC_CR_ADSTART;
            seq_ready = 0;

#ifdef VER_1_0
            Update_TIM14_CH1 (512);		//permito 1.75V en LM311
#endif
            ChangeLed(LED_STANDBY);
            break;

        case MAIN_SYNCHRO_ADC:
            if (seq_ready)
            {
                Usart1Send((char *) (const char *) "ADC Sync getted!\r\n");
                main_state = MAIN_SET_ZERO_CURRENT;
                seq_ready = 0;
                timer_standby = 2000;                
            }
            break;

        case MAIN_SET_ZERO_CURRENT:
            if (!timer_standby)
            {
                //a esta altura debo tener bien medida la tension de alimentacion para poder determina dmax
                d = 0;
                main_state = MAIN_GENERATING;
                ChangeLed(LED_GENERATING);
                EXTIOn();
            }                
            break;

        case MAIN_GENERATING:
            if (!STOP_JUMPER)
            {
                if (seq_ready)
                {
                    seq_ready = 0;

                    if (undersampling < UNDERSAMPLING_TICKS)
                        undersampling++;
                    else
                    {
                        d = PID_roof (VOUT_200V, Vout_Sense, d, &ez1, &ez2);
                    
                        if (d < 0)
                            d = 0;

                        if (d > dmax)
                            d = dmax;

                        UpdateTIMSync (d);
                        dmax = UpdateDMAX(vin_filtered);    //TODO: luego meter el filtro en sync con mustras
                    }
                }    //cierra sequence
            }
            else
            {
                UpdateTIMSync (0);
                d = 0;
                timer_standby = 300;    //doy minimo 300ms para reactivar
                ChangeLed(LED_PROTECTED);
                main_state = MAIN_JUMPER_PROTECTED;
            }            
            break;

        case MAIN_JUMPER_PROTECTED:
            if (!timer_standby)
            {
                if (!STOP_JUMPER)
                {
                    //vuelvo a INIT
                    main_state = MAIN_SET_ZERO_CURRENT;
                    ChangeLed(LED_STANDBY);
                }
            }                
            break;

        case MAIN_OVERCURRENT:
            if (!timer_standby)
            {
                timer_standby = 100;
                if (LED)
                    LED_OFF;
                else
                    LED_ON;
            }

            if (STOP_JUMPER)
                main_state = MAIN_SET_ZERO_CURRENT;
            break;

        default:
            seq_ready = 0;            
            main_state = MAIN_SYNCHRO_ADC;
            break;
        }	//fin switch main_state

        if (!timer_standby)
        {
            timer_standby = 2000;
            sprintf (s_lcd, "VIN: %d, VOUT: %d, d: %d\r\n", Vin_Sense, Vout_Sense, d);
            Usart1Send(s_lcd);
        }

        if (current_excess)
        {
            current_excess = 0;
            d = 0;
            Usart1Send("\r\n Overcurrent!");
            main_state = MAIN_OVERCURRENT;
        }

        if (!timer_filters)
        {
            //espero un poco entre cada muestra de la tension
            timer_filters = 3;
            vin_vector[0] = Vin_Sense;
            vin_filtered = MAFilter8(vin_vector);
        }
            
            

        UpdateLed();
        
    }	//fin while 1
#endif //PRODUCTION_PRGRM

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

    if (timer_led)
        timer_led--;

    if (timer_filters)
        timer_filters--;
    
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

//hubo sobrecorriente, me llaman desde la interrupcion
void Overcurrent_Shutdown (void)
{
    //primero freno todos los PWM
    UpdateTIMSync(0);

    DISABLE_TIM3;
    DISABLE_TIM1;

    LED_ON;    //avio con el led una vez que freno los pwm   

    //ahora aviso del error
    current_excess = 1;
}



#ifdef VER_1_1
void EXTI4_15_IRQHandler(void)
{
    Overcurrent_Shutdown();
    if ((EXTI->PR & 0x00000020) || (EXTI->PR & 0x00000010))	//Line4 or Line5
    {
        EXTI->PR |= 0x00000030;    //4 or 5
    }
}
#endif

#ifdef VER_1_0
void EXTI0_1_IRQHandler(void)
{
    if(EXTI->PR & 0x00000001)	//Line0
    {
        UpdateTIMSync(0);


        current_excess = 1;


        EXTI->PR |= 0x00000001;
    }
}
#endif


//------ EOF -------//
