#include "stm32f4xx_hal.h"
#include <cstring>
#include <CMSIS_RTOS_V2/cmsis_os.h>
#include <error.h>
#include <trace.h>

#define LED_PIN                                GPIO_PIN_5
#define LED_GPIO_PORT                          GPIOA
#define LED_GPIO_CLK_ENABLE()                  __HAL_RCC_GPIOA_CLK_ENABLE()
#define TRANSMIT_TIMEOUT 5

osThreadId_t LED_BlinkingThreadHandle;
osThreadAttr_t LED_BlinkingThread_attributes;

UART_HandleTypeDef huart3;

void LED_Init();
void Start_LED_BlinkingThread(void * argument);
void Error_Handler(void);

static void SetThreadAttributes(
						osThreadAttr_t * attr,
						const char *name,
						uint32_t stack_size,
						osPriority_t prio
					) {
	memset(attr, 0, sizeof(osThreadAttr_t));
	attr->name = name;
	attr->stack_size = stack_size;
	attr->priority = prio;    
}

static void USART3_Init(void)
{
	huart3.Instance = USART3;
	huart3.Init.BaudRate = 115200;
	huart3.Init.WordLength = UART_WORDLENGTH_8B;
	huart3.Init.StopBits = UART_STOPBITS_1;
	huart3.Init.Parity = UART_PARITY_NONE;
	huart3.Init.Mode = UART_MODE_TX_RX;
	huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart3.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart3) != HAL_OK) {
		Error_Handler();
	}
}

int main(void) {
    HAL_Init();
    LED_Init();
    USART3_Init();
    NVIC_SetPriorityGrouping(0);
    osKernelInitialize();

	SetThreadAttributes(
					&LED_BlinkingThread_attributes,
					"LED_BlinkingThread",
					128,
					osPriorityNormal
				);

    LED_BlinkingThreadHandle = osThreadNew(Start_LED_BlinkingThread, NULL, &LED_BlinkingThread_attributes);

    TRACE("Starting kernel...");

	osStatus_t status = osKernelStart();
	if (status != osOK) {
		TRACE("osKernelStart() returned error code: ", status);
	}

    while (1)
    {}
}

void LED_Init() {
  LED_GPIO_CLK_ENABLE();
  GPIO_InitTypeDef GPIO_InitStruct;
  GPIO_InitStruct.Pin = LED_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
  HAL_GPIO_Init(LED_GPIO_PORT, &GPIO_InitStruct);
}

void Start_LED_BlinkingThread(void* argument) {
    (void)argument;
    while (1)
    {
        HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_PIN);
        HAL_Delay(1000);
    }
}
#if 0
void SysTick_Handler(void) {
  HAL_IncTick();
}
#endif
void Error_Handler(void)
{
	__disable_irq();
    std::error_code ec = make_system_error(EFAULT);
	TRACE("Error_Handler");
    if (ec.value()) {
        TRACE(ec.message());
    }
	while (1) ;
}

extern "C" int __io_putchar(int ch)
{
	HAL_UART_Transmit(&huart3, (uint8_t *)&ch, sizeof(uint8_t), TRANSMIT_TIMEOUT);
	return ch;
}
#if 0
extern "C" void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (htim->Instance == TIM1) {
		HAL_IncTick();
	}
}
#endif
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line) {
	TRACE("assert_failed");
}
#endif /* USE_FULL_ASSERT */