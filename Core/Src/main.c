/* ============================
 * File: main.c
 * Description: Elevator Control Main Logic (based on command spec)
 * ============================
 */

#include "main.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "usb_device.h"
#include "usbd_cdc_if.h"

TIM_HandleTypeDef htim2;
UART_HandleTypeDef huart1;

#define MAX_FLOOR 8
#define NUM_CAR 3
#define MAX_QUEUE 10

typedef struct {
    uint8_t currFloor;
    uint8_t targetFloor;
    uint8_t isMoving;
    uint16_t moveTimeMs;
    uint16_t tick;
    uint16_t totalWork;
} CarState;

CarState cReg[NUM_CAR]; // ลิฟต์ทั้ง 3 ตัว

uint8_t userFloor = 0;
uint8_t userReq = 0;

void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_USART1_UART_Init(void);
void MX_TIM2_Init(void);

void updateLED(uint8_t car);

void sendCommandToSB(const char* cmd) {
    CDC_Transmit_FS((uint8_t*)cmd, strlen(cmd));
    HAL_UART_Transmit(&huart1, (uint8_t*)cmd, strlen(cmd), 100);
}

void moveCar(uint8_t car, uint8_t from, uint8_t to) {
    char cmd[32];
    if (from < to)
        snprintf(cmd, sizeof(cmd), "UP,%d,%d\r\n", car, to);
    else if (from > to)
        snprintf(cmd, sizeof(cmd), "Down,%d,%d\r\n", car, to);
    else return;
    sendCommandToSB(cmd);
}


void handleUserRequest(uint8_t from, uint8_t to) {
    handleCall(from, to);
}

void handleCall(uint8_t from, uint8_t to) {
    int bestCar = -1;
    int bestScore = 255;

    for (int i = 0; i < NUM_CAR; i++) {
        int dist = abs(cReg[i].currFloor - from);
        int score = dist + (cReg[i].totalWork / 2);
        if (score < bestScore) {
            bestScore = score;
            bestCar = i;
        }
    }

    if (bestCar >= 0 && from != to) {
        CarState* car = &cReg[bestCar];
        car->targetFloor = from;
        car->isMoving = 1;
        car->tick = 0;
        car->moveTimeMs = 2000;
        car->totalWork += abs(car->currFloor - from);
        moveCar(bestCar, car->currFloor, from);
        userFloor = from;
        userReq = to;
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM2) {
        for (int i = 0; i < NUM_CAR; i++) {
            CarState* car = &cReg[i];
            if (car->isMoving) {
                car->tick += 100;
                if (car->tick >= car->moveTimeMs) {
                    car->tick = 0;
                    if (car->currFloor < car->targetFloor) car->currFloor++;
                    else if (car->currFloor > car->targetFloor) car->currFloor--;

                    if (car->currFloor == car->targetFloor) {
                        if (car->currFloor == userFloor && userFloor != userReq) {
                            car->targetFloor = userReq;
                            car->isMoving = 1;
                            car->tick = 0;
                            car->totalWork += abs(car->currFloor - userReq);
                            moveCar(i, car->currFloor, userReq);
                            userFloor = userReq; // reset trigger
                        } else {
                            car->isMoving = 0;
                        }
                    }
                    updateLED(i);
                }
            }
        }
    }
}

void updateLED(uint8_t car) {
    if (car >= 3) return;

    uint8_t floor = cReg[car].currFloor;

    if (car == 0) {
        for (int f = 0; f < 8; f++) HAL_GPIO_WritePin(GPIOA, 1 << f, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOA, 1 << floor, GPIO_PIN_SET);
    }

    else if (car == 1) {
        for (int f = 0; f < 8; f++) HAL_GPIO_WritePin(GPIOB, 1 << f, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, 1 << floor, GPIO_PIN_SET);
    }

    else if (car == 2) {
        GPIO_TypeDef* ports[8] = {GPIOB, GPIOB, GPIOB, GPIOB, GPIOB, GPIOB, GPIOB, GPIOA};
        uint16_t pins[8] = {
            GPIO_PIN_8, GPIO_PIN_9, GPIO_PIN_10,
            GPIO_PIN_12, GPIO_PIN_13, GPIO_PIN_14, GPIO_PIN_15,
            GPIO_PIN_8
        };

        for (int i = 0; i < 8; i++) {
            HAL_GPIO_WritePin(ports[i], pins[i], GPIO_PIN_RESET);
        }

        HAL_GPIO_WritePin(ports[floor], pins[floor], GPIO_PIN_SET);
    }
}
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM2_Init();
    MX_USART1_UART_Init();
    MX_USB_DEVICE_Init();

    for (int i = 0; i < NUM_CAR; i++) {
        cReg[i].currFloor = 0;
        cReg[i].targetFloor = 0;
        cReg[i].isMoving = 0;
        cReg[i].moveTimeMs = 2000;
        cReg[i].tick = 0;
        cReg[i].totalWork = 0;
        updateLED(i);
    }

    HAL_TIM_Base_Start_IT(&htim2);
    HAL_UART_Receive_IT(&huart1, 0, 1);

    while (1) {
        // idle
    }
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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 144;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 3;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 7200-1;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 1000-1;
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
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7
                          |GPIO_PIN_8, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_10
                          |GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15
                          |GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6
                          |GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9, GPIO_PIN_RESET);

  /*Configure GPIO pins : PA0 PA1 PA2 PA3
                           PA4 PA5 PA6 PA7
                           PA8 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7
                          |GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB0 PB1 PB2 PB10
                           PB12 PB13 PB14 PB15
                           PB3 PB4 PB5 PB6
                           PB7 PB8 PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_10
                          |GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15
                          |GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6
                          |GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

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

#ifdef  USE_FULL_ASSERT
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
