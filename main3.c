/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "SDBM.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
unsigned char modo = 0;
unsigned short tiempo_echo = 0;
int tiempo;
unsigned char flanco = 0;
unsigned char trigger = 0;
unsigned char led_encendido = 0;
unsigned char giros = 0;
unsigned short distancias[5] = {0,0,0,0,0};
unsigned char dato_listo = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_ADC_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM4_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void EXTI15_10_IRQHandler(void){ // Boton Cambiar Modo PC13
		if(EXTI->PR != 0){
			modo++;
			if(modo > 1) modo = 0;
			EXTI->PR = (0x01 << 13); // Limpiar flags
		}
	}
	void EXTI4_IRQHandler(void){ // Boton iniciar medida PB5
		if(EXTI->PR != 0){

			GPIOB->BSRR |= (1 << 5); // Encendemos el LED de medida
			led_encendido = 1;

			TIM4->CR1 |= 0x01; // Enciendo el TIM4

			// Preparo el apagado del LED a los 0.5s (Canal 1)
			TIM4->CCR1 = TIM4->CNT + 500;
			TIM4->SR &= ~0x0002;

			if (modo == 0) {
				// --- MODO MANUAL ---
				// Solo utilizo el ultrasonidos una vez
				GPIOC->BSRR |= (1 << 8); // Subir Trigger
				trigger = 1;
				TIM3->CNT = 0;
				TIM3->CR1 |= 0x01;       // Enciendo el TIM3

			} else if (modo == 1) {
				// --- MODO AUTOMÁTICO  ---
				giros = 0;               // Reiniciamos el contador del servo
				TIM2->CCR1 = 1000;       // Reinicio la posicion del servo a -90º

				// Primera medida
				GPIOC->BSRR |= (1 << 8);
				trigger = 1;
				TIM3->CNT = 0;
				TIM3->CR1 |= 0x01;

				// Preparo el movimiento para dentro de 2 segundos (Canal 2)
				TIM4->CCR2 = TIM4->CNT + 2000;
				TIM4->SR &= ~0x0004;
			}
			EXTI->PR = (0x01 << 4); // Limpiar flags
		}
	}
	void TIM3_IRQHandler(void) { // Ultrasonidos, TIC (canal 4) y TOC (canal 3)
		if ((TIM3->SR & 0x0010)!=0) { // Si la comparacion es exitosa, la IRQ se lanza
		// y la ISR se ejecuta. Esta linea chequea si la linea lanza el ISR
			if(flanco == 0){
				tiempo_echo = TIM3->CCR4; // Tiempo de primer flanco
				flanco++;
			}else{
				tiempo = TIM3->CCR4 - tiempo_echo; // Tiempo del pulso
				if(tiempo < 0) tiempo += 0xFFFF;
				flanco = 0;
				dato_listo = 1; // Aviso al micro para calcular la distancia
				TIM3->CR1 &= ~0x0001; // Apago el timer hasta la próxima medida
			}
			TIM3->SR &= ~(1 << 4); // Limpio Flags
		}
		if((TIM3->SR & 0x0008) != 0){
			if(trigger){ // Si se sube el trigger entra
				GPIOC->BSRR |= (1 << 8)<<16;
			}
			TIM3->SR &= ~(1 << 3); // Limpio Flags
		}
	}
	void TIM4_IRQHandler(void){ // Contador de tiempo

		if((TIM4->SR & 0x0002) != 0){ // Apagar el LED (0.5s)
			if(led_encendido){
				GPIOB->BSRR |= (1 << (5 + 16));
				led_encendido = 0;
			}
			TIM4->SR &= ~0x0002; // Limpio Flags del Canal 1
		}
		if((TIM4->SR & 0x0004) != 0){ //Tiempo servo auto (2.0s)
			if(modo == 1){
				giros++; // Sumamos un paso al barrido

				if(giros < 5){ // 5 posiciones
					// Muevo el servo al siguiente paso (+45º)
					TIM2->CCR1 += 250;

					// Nueva medida de ultrasonidos
					GPIOC->BSRR |= (1 << 8); // Subir Trigger
					trigger = 1;
					TIM3->CNT = 0;
					TIM3->CR1 |= 0x01;       // Enciendo el TIM3

					// Enciendo el LED otra vez para indicar la nueva medida
					GPIOB->BSRR |= (1 << 5);
					led_encendido = 1;
					TIM4->CCR1 = TIM4->CNT + 500; // Preparo el apagado del LED

					// Preparo la siguiente medida para dentro de 2 segundos
					TIM4->CCR2 = TIM4->CNT + 2000;
				}
			TIM4->SR &= ~(1 << 2); // Limpio Flags del Canal 2
		}
		}
	}
	void on_off_led_distancias(short distancia){ // LEDs distancias
		if(distancia > 80){ 					// Enciendo segun la distancia
		  GPIOC->BSRR |= (1 << (0+16));
		  GPIOC->BSRR |= (1 << (1+16));
		  GPIOC->BSRR |= (1 << (2+16));
		  GPIOC->BSRR |= (1 << (3+16));
		  GPIOC->BSRR |= (1 << (4+16));

		  }else if(distancia < 10){
			  GPIOC->BSRR |= (1 << 0);
			  GPIOC->BSRR |= (1 << 1);
			  GPIOC->BSRR |= (1 << 2);
			  GPIOC->BSRR |= (1 << 3);
			  GPIOC->BSRR |= (1 << 4);
		  }else if(distancia < 60){
			  GPIOC->BSRR |= (1 << 0);
			  GPIOC->BSRR |= (1 << 1);
		  }
	}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
	unsigned char modo_config = 0;
	unsigned short valor = 0;
	unsigned short angulo = 0;
	int8_t diferencia = 0;
	unsigned short distancia = 0;
	uint8_t texto[6];
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_ADC_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */
  	// Configuracion PA5 LED
	GPIOA->MODER &= ~(1 << 5 * 2);
	GPIOA->MODER &= ~(1 << (5 * 2 + 1));
	GPIOA->MODER |= (1 << 5 * 2); // Salida LED
	GPIOA->AFR[0] &= ~(0b1111 << 5 * 4);

	// Configuracion PC13 Boton
	GPIOC->MODER &= ~(1 << 13 * 2);
	GPIOC->MODER &= ~(1 << (13 * 2 + 1)); // Entrada Boton
	GPIOC->AFR[1] &= ~(0b1111 << (13-8) * 4); // Funcion alternativa por defecto
	GPIOC->PUPDR &= ~(0b11 << 13 * 2); // Sin resistencia de pull-up o pull-down
	// Configurando EXTI13 para flanco de bajada
	EXTI->FTSR |= (0x01 << 13); // Activa flanco de bajada en EXTI13
	EXTI->RTSR &= ~(0x01 << 13); // Desactiva flanco de subida en EXTI13
	SYSCFG->EXTICR[3] = (2 << 4); // EXTI13 unido a GPIOC (i.e. USER button = PC13)
	EXTI->IMR |= (0x01 << 13); // Habilita EXTI13
	NVIC->ISER[1] |= (1 << (40 - 32)); // Habilita IRQ

	// Boton inicio de medida
	GPIOB->MODER &= ~(0b11 << 4);
	GPIOB->PUPDR |= (1 << (4 * 2));
	GPIOB->PUPDR &= ~(1 << (4 * 2 + 1));
	// Configurando EXTI4 para flanco de bajada
	EXTI->FTSR |= (0x01 << 4); // Activa flanco de bajada en EXTI4
	EXTI->RTSR &= ~(0x01 << 4); // Desactiva flanco de subida en EXTI4
	SYSCFG->EXTICR[1] |= (1 << 0); // EXTI4 unido a GPIOB (i.e. boton medida = PB4)
	EXTI->IMR |= (1 << 4); // Habilita EXTI4
	EXTI->PR = (0x01 << 4); // Limpiar flags para que no salte la interrupcion antes de tiempo
	NVIC->ISER[0] |= (1 << 10); // Habilita IRQ

	// LED (indicador medir distacias)
	GPIOB->MODER &= ~(0b11 << 5);
	GPIOB->MODER |= (1 << (5 * 2));

	// Configuracion contadores TOC canales 1 y 2
	// Config. reloj interno: CR1, CR2, SMRC
	TIM4->CR1 = 0x0000; // ARPE = 0 -> No es PWM, es TOC
	// CEN = 0 , Contador Apagado
	TIM4->CR2 = 0x0000; // Siempre a cero
	TIM4->SMCR = 0x0000; // Siempre a cero
	// Config. contador: PSC, CNT, ARR y CCRx
	TIM4->CNT = 0; // Contador a cero
	TIM4->PSC = 32000 - 1; // Preescalado para contar 1000 pasos/segundo
	TIM4->ARR = 0xFFFF;
	// Seleccion de IRQ: DIER
	TIM4->DIER = 0x0006; // Interrupcion canal 1 y 2
	// Modo config.
	TIM4->CCMR1 = 0x0000; // CC4S = 01 (TIC), CC3S = 00 (TOC)
	TIM4->CCER = 0x0000; // CC3P = 0, CC3NP = 0
						// CC3E = 0
	// Encendiendo contador
	TIM4->SR = 0x0000; // Limpiar flags
	TIM4->EGR |= 0x0001;
	NVIC->ISER[0] |= (1 << 30); // Habilitar interrupcion para TIM4

	// Potenciometro
	GPIOA->MODER &= ~(0b11 << 1);
	GPIOA->MODER |= (1 << 1 * 2); // Configurar como analógico
	GPIOA->MODER |= (1 << (1 * 2 + 1));
	// Configuracion ADC conversion simple
	ADC1->CR2 &= ~(0x00000001); // ADON = 0 (ADC apagado)
	ADC1->CR1 = 0x00000000; // OVRIE = 0 (overrun IRQ desactivado)
	// RES = 00 (resolucion = 12 bits)
	// SCAN = 0 (scan mode desactivado)
	// EOCIE = 0 (End of conversion IRQ desactivado)
	ADC1->CR2 = 0x00000400; // EOCS = 1 (End of Conversion activado despues de cada conversion)
	// DELS = 000 (No hay retardo)
	// CONT = 0 (Conversion Simple)
	ADC1->SQR1 = 0x00000000; // 1 canal en la sequencia
	ADC1->SQR5 = 0x00000001; // EL canal es AIN1
	ADC1->CR2 |= 0x00000001; // ADON = 1 (ADC encendido)
	while ((ADC1->SR&0x0040)==0); // Mientras ADONS = 0, i.e
								 // Para convertir, espero

	// Configuracion LEDs indicador distancias
	GPIOC->MODER &= ~(0b11 << 0);
	GPIOC->MODER &= ~(0b11 << 1);
	GPIOC->MODER &= ~(0b11 << 2);
	GPIOC->MODER &= ~(0b11 << 3);
	GPIOC->MODER &= ~(0b11 << 4);
	GPIOC->MODER |= (1 << (0 * 2));
	GPIOC->MODER |= (1 << (1 * 2));
	GPIOC->MODER |= (1 << (2 * 2));
	GPIOC->MODER |= (1 << (3 * 2));
	GPIOC->MODER |= (1 << (4 * 2));

	// Configuracion Ultrasonidos
	// Trigger
	GPIOC->MODER &= ~(0b11 << 8);
	GPIOC->MODER |= (1 << 8 * 2);
	// Echo
	GPIOC->MODER &= ~(0b11 << 9);
	GPIOC->MODER |= (1 << (9 * 2 + 1));
	GPIOC->AFR[1] |= (1 << ((9 - 8) * 4 + 1)); // Configurando AF2 con PC9
	//TIM3 canañ 4 TIC y canal 3 TOC
	// Config. reloj interno: CR1, CR2, SMRC
	TIM3->CR1 = 0x0000; // ARPE = 0 -> No PWM, es Tic
	 // CEN = 0; Contador OFF
	TIM3->CR2 = 0x0000; // Siempre 0
	TIM3->SMCR = 0x0000; // Siempre 0
	// Config. contador: PSC, CNT, ARR y CCRx
	TIM3->PSC = 32 - 1; // Preescalado = 32 -> f_counter=32000000/32 = 1 pasos/microsegundo
	TIM3->CNT = 0; // Inicializo el contador a 0
	TIM3->ARR = 0xFFFF;
	TIM3->CCR3 = 15;
	TIM3->CCR4 = 0; // Se almacena el valor contado (en TIC no hace falta instanciarlo)
	// Seleccion de IRQ: DIER
	TIM3->DIER = 0x0018; // Hay IRQ cuando la cuenta ha terminado -> CC3IE = 1
						// Hay IRQ cuando la cuenta ha terminado -> CC4IE = 1
	// Modo config.
	TIM3->CCMR2 = 0x0100; // CC4S = 01 (TIC), CC3S = 00 (TOC)
	TIM3->CCER = 0xB000; // CC3P = 0, CC3NP = 0
						 // CC3E = 0
						 // CC4P = 1, CC4NP = 1 (por ambos flancos)
						 // CC4E = 1 (Salida hardware activada)
	// Encendiendo contador
	TIM3->EGR |= 0x0001; // UG = 1 -> Genero un evento de actualizacion
					// para actualizar los registros
	TIM3->SR = 0; // Limpio flags
	// Habilitar la IRQ en TIM3
	NVIC->ISER[0] |= (1 << 29);

	// Configuracion PWM canal 1 PA0
	GPIOA->MODER &= ~(0b11 << 0);
	GPIOA->MODER |= (0b10 << 0);
	GPIOA->AFR[0] |= (1 << 0); // Configuracion AF1 en PA0
	// Config. reloj interno: CR1, CR2, SMRC
	TIM2->CR1 = 0x0080; // ARPE = 1 -> Es PWM
	 // CEN = 0; Contador OFF
	TIM2->CR2 = 0x0000; // Siempre 0
	TIM2->SMCR = 0x0000; // Siempre 0
	// Config. contador: PSC, CNT, ARR y CCRx
	TIM2->PSC = 32 - 1; // Preescalado = 32 -> f_counter=32000000/32 = 1 pasos/microsegundo
	TIM2->CNT = 0; // Inicializo el contador a 0
	TIM2->ARR = 20000 - 1;
	TIM2->CCR1 = 1000;
	// Seleccion de IRQ: DIER
	TIM2->DIER = 0x0000; // No hay IRQ es PWM
	// Modo config.
	TIM2->CCMR1 = 0x0068; // CC1S = 00
	TIM2->CCER = 0x0001; // CC1P = 0, CC1NP = 0
						 // CC1E = 1
	// Encendiendo contador
	TIM2->CR1 |= 0x0001;	 // CEN = 1 -> Empezar Contador
	TIM2->EGR |= 0x0001; // UG = 1 -> Genero un evento de actualizacion
				// para actualizar los registros
	TIM2->SR = 0; // Limpio flags

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  if(modo != modo_config){
		  modo_config = modo;
		  switch(modo){
		  case 1:
			  GPIOA->BSRR |= (1 << 5);
			  printf("Modo Automático\r\n");
			 break;
		  default:
			  GPIOA->BSRR |= (1 << (5 + 16));
			  printf("Modo Manual\r\n");
			  break;
			  }
		  }
	  if(dato_listo == 1){
		  if(modo_config == 0){
			  printf("midiendo...\n\r");
			  // Empiezo la conversion
			  ADC1->CR2 |= 0x40000000; // Cuando ADONS = 1, empiezo la conversion
			  // (SWSTART = 1)
			  // Espero hasta que la conversion acaba
			  while ((ADC1->SR&0x0002)==0); // Si EOC = 0, i.e., la conversion no
			  // acaba, espero
			  valor = ADC1->DR; // Cuando End of Conversion = 1, guardo el resultado y lo almaceno en valor
			  angulo = 1000 + ((valor * 1000) / 4095); // Cálculo del DC
			  diferencia = angulo - TIM2->CCR1;
			  if(diferencia < 0) diferencia = -diferencia;
			  if(diferencia > 20){
				  TIM2->CCR1 = angulo;
			  }
			  Bin2Ascii(valor,&texto[0]);
			  // Ensenho el resultado en Tera Term
			  printf((uint8_t*)texto);
			  printf("\n\r");
			  distancia = tiempo / 58; // Calcular distancia medida con ultrasonidos
			  printf("Modo Manual -> Distancia: %i cm\n\r", distancia);
			  on_off_led_distancias(distancia); // Actualizar los LEDs con la distancia actual
			  dato_listo = 0;
		  }else if(modo_config == 1){
			  printf("midiendo...\n\r");

			  distancia = tiempo / 58; // Calcular distancia medida con ultrasonidos
			  // Aseguramos el valor del índice
			  if(giros >= 0 && giros < 5){
				  distancias[giros] = distancia; // Guardamos el dato
			  }
			  // Si acabamos de medir la última posición (giros == 4), imprimimos
			  if(giros == 4){
				  printf("Modo Automático -> Distancia: [%i, %i, %i, %i, %i] cm\n\r", distancias[0], distancias[1],
						  distancias[2],distancias[3],distancias[4]);
			  }
			  on_off_led_distancias(distancia); // Actualizar los LEDs con la distancia actual
			  dato_listo = 0;
		  }
	  }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  RCC_OscInitStruct.PLL.PLLDIV = RCC_PLL_DIV3;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC_Init(void)
{

  /* USER CODE BEGIN ADC_Init 0 */

  /* USER CODE END ADC_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC_Init 1 */

  /* USER CODE END ADC_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc.Instance = ADC1;
  hadc.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc.Init.Resolution = ADC_RESOLUTION_12B;
  hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  hadc.Init.LowPowerAutoWait = ADC_AUTOWAIT_DISABLE;
  hadc.Init.LowPowerAutoPowerOff = ADC_AUTOPOWEROFF_DISABLE;
  hadc.Init.ChannelsBank = ADC_CHANNELS_BANK_A;
  hadc.Init.ContinuousConvMode = DISABLE;
  hadc.Init.NbrOfConversion = 1;
  hadc.Init.DiscontinuousConvMode = DISABLE;
  hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc.Init.DMAContinuousRequests = DISABLE;
  if (HAL_ADC_Init(&hadc) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_4CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC_Init 2 */

  /* USER CODE END ADC_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65535;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 0;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 65535;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
