#include "mbed.h"
#include "platform/mbed_thread.h"

#define BLINKING_RATE_MS	500
int main()
{
    // Initialise the digital pin LED1 as an output
    DigitalOut led(LED1);

    while (true) {
        led = !led;
        thread_sleep_for(BLINKING_RATE_MS);
    }
}