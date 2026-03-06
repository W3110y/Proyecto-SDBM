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

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
unsigned char modo = 0;
	unsigned char medida = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_ADC_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void EXTI15_10_IRQHandler(void){
		if(EXTI->PR != 0){
			modo++;
			if(modo > 1) modo = 0;
			EXTI->PR = (0x01 << 13); // Limpiar flags
		}
	}
	void EXTI4_IRQHandler(void){
			if(EXTI->PR != 0){
				medida++;
				GPIOB->BSRR |= (1 << 5);
				EXTI->PR = (0x01 << 4); // Limpiar flags
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
		unsigned char medida_act = 0;
		unsigned short valor = 0;
		//uint8_t angulo = 0;
		uint8_t texto[6];
		uint8_t angulos[5] = {-90, -45, 0, 45, 90};
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
	GPIOB->MODER |= (1 << 5* 2);
	GPIOB->MODER &= ~(1 << (5 * 2 + 1));
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

	// Configuracion Timers
	
	
	printf("Modo Manual\r\n");
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
	  if(medida != medida_act){
		  medida_act = medida;
			  if(modo_config == 0 && medida_act == 1){
				  printf("midiendo...\n\r");
				  // Empiezo la conversion
				  ADC1->CR2 |= 0x40000000; // Cuando ADONS = 1, empiezo la conversion
				  // (SWSTART = 1)
				  // Espero hasta que la conversion acaba
				  while ((ADC1->SR&0x0002)==0); // Si EOC = 0, i.e., la conversion no
				  // acaba, espero
				  valor = ADC1->DR; // Cuando End of Conversion = 1, guardo el resultado y lo almaceno en valor
				  Bin2Ascii(valor, &texto[0]);

				  //angulo = (texto* 180) / 4095; // Cálculo del angulo
				  // Ensenho el resultado en Tera Term
				  printf((uint8_t*)texto);
				  printf("\n\r");
				  GPIOB->BSRR |= (1 << 5)<<16;
				  medida = 0;
			  }else if(modo_config == 1 && medida_act == 1){
				  printf("midiendo...\n\r");
				  for(int i = 0;i<5;i++){
					  printf("%i",angulos[i]);
					  printf("\n\r");
				  }
				  GPIOB->BSRR |= (1 << 5)<<16;
				  medida = 0;
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
