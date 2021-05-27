#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

#include "consts.h"
#include "dns_resolver.h"
#include "error.h"
#include "packet.h"
#include "socket.h"
#include "connection.h"

#include <iostream>
#include <string>

using namespace std;
using namespace coap;

extern "C" void app_main()
{
    cout << "Hello world!\n";
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    cout << "silicon revision: " <<  (int)chip_info.revision << "\n";
    cout << spi_flash_get_chip_size() / (1024 * 1024) << "MB "
    << ((chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external")
    << " flash" << "\n";
    cout << "This CPU has " << (is_little_endian_byte_order() ? "Little" : "Big")
    <<  " Endin architecture\n";
    cout << "If it was launched by \"idf.py monitor\" press \"Ctrl+]\" to exit\n";

    while(1)
    {
        cout << "endless loop\n";
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

