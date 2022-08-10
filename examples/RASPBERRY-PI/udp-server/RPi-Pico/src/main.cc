#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/sync.h"

#include "FreeRTOS.h"
#include "task.h"

#include "ADXL345.h"
#include "pico_protocol.h"

// SPI Defines
// We are going to use SPI 0, and allocate it to the following GPIO pins
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define SPI_PORT spi0
#define PIN_MISO 4
#define PIN_CS   5
#define PIN_SCK  2
#define PIN_MOSI 3

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define I2C_PORT i2c0
#define I2C_SDA 8
#define I2C_SCL 9

// FreeRTOS Defines
#define LED_TASK_STACK_SIZE     (configMINIMAL_STACK_SIZE + sizeof(const uint) + sizeof(bool))
#define LED_TASK_PRIO           (tskIDLE_PRIORITY + 1)
#define ADXL345_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE + 3 * sizeof(int16_t) + sizeof(i2c_inst_t *) + sizeof(class Adxl345))
#define ADXL345_TASK_PRIO       (tskIDLE_PRIORITY + 1)
#define PICO_PROTO_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE + sizeof(spi_inst_t *) + sizeof(std::error_code) +  sizeof(class PicoProtocol))
#define PICO_PROTO_TASK_PRIO       (tskIDLE_PRIORITY + 1)

// extern C functions
#define EXTERN_C extern "C"

EXTERN_C void vApplicationMallocFailedHook(void);
EXTERN_C void vApplicationIdleHook(void);
EXTERN_C void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName);
EXTERN_C void vApplicationTickHook( void );

static void hardware_init(void)
{
    stdio_init_all();

    // SPI initialisation. It will use SPI at 1 MHz.
    spi_init(SPI_PORT, 1000*1000);
    spi_set_slave(SPI_PORT, true);
    spi_set_format (SPI_PORT, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
 
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_CS,   GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);  

    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400*1000);
    
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}

static void LED_flashing_task(void *parameters)
{
    const uint LED_PIN = *((const uint *)parameters);
    bool state = true;

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
    while(true)
    {
        gpio_put(LED_PIN, (uint32_t)state);
        state = !state;
        printf("Core %d - Thread '%s'\n", get_core_num(), pcTaskGetName(xTaskGetCurrentTaskHandle()));
        vTaskDelay(pdMS_TO_TICKS( 500 ));
    }
}

static void ADXL345_pooling_task(void *parameters)
{
    int16_t accelerometer_data[3];
    i2c_inst_t *i2c = (i2c_inst_t *)parameters;
    if (i2c == nullptr)
        vTaskDelete(xTaskGetCurrentTaskHandle());

    memset(accelerometer_data, 0, sizeof(accelerometer_data));

    Adxl345 sensor(i2c);

    if (!sensor.init())
    {
        puts("adxl345 initialization failed\n");
        vTaskDelete(xTaskGetCurrentTaskHandle());
    }

    while(true)
    {
        sensor.read(accelerometer_data, sizeof(accelerometer_data));
        printf("Core %d - Thread '%s'\n", get_core_num(), pcTaskGetName(xTaskGetCurrentTaskHandle()));
        //3.9mg/LSB scale factor in 13-bit mode
        printf("ACCEL: %f\t%f\t%f\n",
                        ((float)accelerometer_data[0])*3.9/1000,
                        ((float)accelerometer_data[1])*3.9/1000,
                        ((float)accelerometer_data[2])*3.9/1000);
        vTaskDelay(pdMS_TO_TICKS( 500 ));
    }
}

static void pico_protocol_task(void *parameters)
{
    spi_inst_t *spi = (spi_inst_t *)parameters;
    printf("spi = %p\n", spi);
    std::error_code ec;
    if (spi == nullptr)
        vTaskDelete(xTaskGetCurrentTaskHandle());

    PicoProtocol proto(spi, nullptr); 
   
    while(true)
    {
        proto.process(ec);
        if (ec.value())
            printf("process() error : %s", ec.message());

        printf("Core %d - Thread '%s'\n", get_core_num(), pcTaskGetName(xTaskGetCurrentTaskHandle()));
        taskYIELD();
    }
}

static void FreeRTOS_init()
{
#if 0
    uint LED_pin = PICO_DEFAULT_LED_PIN;
    xTaskCreate(
            LED_flashing_task,
            "LED",
            LED_TASK_STACK_SIZE,
            &LED_pin,
            LED_TASK_PRIO,
            NULL);

    xTaskCreate(
            ADXL345_pooling_task,
            "ADXL345",
            ADXL345_TASK_STACK_SIZE,
            I2C_PORT,
            ADXL345_TASK_PRIO,
            NULL);
#endif
    printf("SPI_PORT=%p\n", SPI_PORT);
    xTaskCreate(
            pico_protocol_task,
            "Protocol",
            PICO_PROTO_TASK_STACK_SIZE,
            SPI_PORT,
            PICO_PROTO_TASK_PRIO,
            NULL);
}

int main()
{
    hardware_init();

    puts("FreeRTOS initializing...");

    FreeRTOS_init();

    puts("Done\n");
    
    puts("Launching FreeRTOS scheduler...\n");

    vTaskStartScheduler();

    /* should never reach here */
    panic_unsupported();

    return 0;
}

EXTERN_C void vApplicationMallocFailedHook( void )
{
    /* Called if a call to pvPortMalloc() fails because there is insufficient
    free memory available in the FreeRTOS heap.  pvPortMalloc() is called
    internally by FreeRTOS API functions that create tasks, queues, software
    timers, and semaphores.  The size of the FreeRTOS heap is set by the
    configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */

    /* Force an assert. */
    configASSERT( ( volatile void * ) NULL );
}

EXTERN_C void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
    ( void ) pcTaskName;
    ( void ) pxTask;

    /* Run time stack overflow checking is performed if
    configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
    function is called if a stack overflow is detected. */

    /* Force an assert. */
    configASSERT( ( volatile void * ) NULL );
}

EXTERN_C void vApplicationIdleHook( void )
{
    volatile size_t xFreeHeapSpace;

    /* This is just a trivial example of an idle hook.  It is called on each
    cycle of the idle task.  It must *NOT* attempt to block.  In this case the
    idle task just queries the amount of FreeRTOS heap that remains.  See the
    memory management section on the http://www.FreeRTOS.org web site for memory
    management options.  If there is a lot of heap memory free then the
    configTOTAL_HEAP_SIZE value in FreeRTOSConfig.h can be reduced to free up
    RAM. */
    xFreeHeapSpace = xPortGetFreeHeapSize();

    /* Remove compiler warning about xFreeHeapSpace being set but never used. */
    ( void ) xFreeHeapSpace;
}

EXTERN_C void vApplicationTickHook( void )
{
}
