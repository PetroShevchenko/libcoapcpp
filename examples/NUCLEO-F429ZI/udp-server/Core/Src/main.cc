#include <string.h>
#include "main.h"
#include "cmsis_os.h"
#include "log.h"
#include "lwip.h"
#include "dhcp.h"
#include "udp_server.hh"
#include "command.hh"

#ifndef DHCP_STATE_BOUND
#define DHCP_STATE_BOUND 10
#endif

#define UDP_PORT_NUM 5555UL

using namespace std;

void Error_Handler(void);
int main(void);

osThreadAttr_t IpAssignerTask_attributes;
osThreadAttr_t UdpServerTask_attributes;

RTC_HandleTypeDef hrtc;
UART_HandleTypeDef huart3;
PCD_HandleTypeDef hpcd_USB_OTG_FS;

osThreadId_t IpAssignerTaskHandle;
osThreadId_t UdpServerTaskHandle;

void SystemClock_Config(void);
void StartIpAssignerTask(void *argument);
void StartUdpServerTask(void *argument);

static void GPIO_Init(void);
static void USART3_UART_Init(void);
static void USB_OTG_FS_PCD_Init(void);
static void RTC_Init(void);
static void SetThreadAttributes(
						osThreadAttr_t * attr,
						const char *name,
						uint32_t stack_size,
						osPriority_t prio
					);

static void GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOG_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOB, LD1_Pin|LD3_Pin|LD2_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(USB_PowerSwitchOn_GPIO_Port, USB_PowerSwitchOn_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin : USER_Btn_Pin */
	GPIO_InitStruct.Pin = USER_Btn_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(USER_Btn_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pins : LD1_Pin LD3_Pin LD2_Pin */
	GPIO_InitStruct.Pin = LD1_Pin|LD3_Pin|LD2_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*Configure GPIO pin : USB_PowerSwitchOn_Pin */
	GPIO_InitStruct.Pin = USB_PowerSwitchOn_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(USB_PowerSwitchOn_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : USB_OverCurrent_Pin */
	GPIO_InitStruct.Pin = USB_OverCurrent_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(USB_OverCurrent_GPIO_Port, &GPIO_InitStruct);
}

static void USART3_UART_Init(void)
{
	huart3.Instance = USART3;
	huart3.Init.BaudRate = 115200;
	huart3.Init.WordLength = UART_WORDLENGTH_8B;
	huart3.Init.StopBits = UART_STOPBITS_1;
	huart3.Init.Parity = UART_PARITY_NONE;
	huart3.Init.Mode = UART_MODE_TX_RX;
	huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart3.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart3) != HAL_OK)
	{
		Error_Handler();
	}
}

static void USB_OTG_FS_PCD_Init(void)
{
	hpcd_USB_OTG_FS.Instance = USB_OTG_FS;
	hpcd_USB_OTG_FS.Init.dev_endpoints = 4;
	hpcd_USB_OTG_FS.Init.speed = PCD_SPEED_FULL;
	hpcd_USB_OTG_FS.Init.dma_enable = DISABLE;
	hpcd_USB_OTG_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
	hpcd_USB_OTG_FS.Init.Sof_enable = ENABLE;
	hpcd_USB_OTG_FS.Init.low_power_enable = DISABLE;
	hpcd_USB_OTG_FS.Init.lpm_enable = DISABLE;
	hpcd_USB_OTG_FS.Init.vbus_sensing_enable = ENABLE;
	hpcd_USB_OTG_FS.Init.use_dedicated_ep1 = DISABLE;
	if (HAL_PCD_Init(&hpcd_USB_OTG_FS) != HAL_OK)
	{
		Error_Handler();
	}
}

static void RTC_Init(void)
{
  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
	Error_Handler();
  }
  sTime.Hours = 0;
  sTime.Minutes = 0;
  sTime.Seconds = 0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
  sDate.Month = RTC_MONTH_JANUARY;
  sDate.Date = 1;
  sDate.Year = 0;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
}

static void SetThreadAttributes(
					osThreadAttr_t * attr,
					const char *name,
					uint32_t stack_size,
					osPriority_t prio
				)
{
	memset(attr, 0, sizeof(osThreadAttr_t));
	attr->name = name;
	attr->stack_size = stack_size;
	attr->priority = prio;
}

int main(void)
{
	HAL_Init();

	SystemClock_Config();

	GPIO_Init();
	USART3_UART_Init();
	USB_OTG_FS_PCD_Init();
	RTC_Init();
	LWIP_Init();

	NVIC_SetPriorityGrouping(0);

	osKernelInitialize();

	SetThreadAttributes(
					&IpAssignerTask_attributes,
					"IpAssignerTask",
					128 * 4,
					osPriorityNormal
				);

	SetThreadAttributes(
					&UdpServerTask_attributes,
					"UdpServerTask",
					1024 * 4,
					osPriorityLow
				);

	init_log();

	IpAssignerTaskHandle = osThreadNew(StartIpAssignerTask, NULL, &IpAssignerTask_attributes);
	UdpServerTaskHandle = osThreadNew(StartUdpServerTask, NULL, &UdpServerTask_attributes);

	osStatus_t status = osKernelStart();
	if ( status != osOK)
	{
		LOG("osKernelStart() returned error %d", status);
	}

	while(1) ;
}

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
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_LSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
	RCC_OscInitStruct.LSEState = RCC_LSE_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 4;
	RCC_OscInitStruct.PLL.PLLN = 168;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 7;
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
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
	{
		Error_Handler();
	}
}

void StartIpAssignerTask(void *argument)
{
	struct dhcp *dhcp;
	char msg[16];
	bool dhcp_bound_flag = false;
	uint32_t offered_ip = 0;
	(void)argument;

	for(;;)
	{
		dhcp = netif_dhcp_data(lwip_get_netif());
		if (dhcp->state == DHCP_STATE_BOUND && !dhcp_bound_flag)
		{
		  dhcp_bound_flag = true;
		  offered_ip = ip4_addr_get_u32(&dhcp->offered_ip_addr);
		  snprintf(msg, sizeof(msg), "%03lu.%03lu.%03lu.%03lu",
				  (offered_ip)&0xFF,  (offered_ip >> 8)&0xFF, (offered_ip >> 16)&0xFF, (offered_ip >> 24)&0xFF);
		  LOG("IP address assigned by DHCP: %s",msg);
		}
		osDelay(1000);
	}
}

void StartUdpServerTask(void *argument)
{
	(void)argument;

	error_code ec;
	static UdpServer server(UDP_PORT_NUM, ec, true);
	if (ec.value())
	{
		LOG("Cannot create UdpServer object: %s", ec.message().c_str());
	}

	osDelay(5000);

	LOG("server object created");
	server.init(ec);
	if (ec.value())
	{
		LOG("server.init() error: %s", ec.message().c_str());
	}
	LOG("set_received_packed_handler_callback");
	server.set_received_packed_handler_callback(static_cast<ReceivedPacketHandlerCallback>(command_handler));
	LOG("start");
	server.start();
	LOG("process2");	
	server.process2();

	while(true)
	{
		osDelay(1);
	}
}

void Error_Handler(void)
{
	__disable_irq();
	LOG("Error_Handler");
	while (1) ;
}

extern "C" int __io_putchar(int ch)
{
	HAL_UART_Transmit(&huart3, (uint8_t *)&ch, sizeof(uint8_t), TRANSMIT_TIMEOUT);
	return ch;
}

extern "C" void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM1) {
		HAL_IncTick();
	}
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
	LOG("assert_failed");
}
#endif /* USE_FULL_ASSERT */
