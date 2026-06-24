#include <libopencm3/stm32/rcc.h>

#include "app/actuators.h"
#include "app/app.h"
#include "app/sensors.h"
#include "app/tasks.h"
#include "drivers/uart_comm.h"
#include "drivers/leds.h"
#include "drivers/buzzer.h"
#include "drivers/adc.h"
#include "drivers/button.h"
#include "drivers/presence.h"
#include "platform/system_init.h"

int main(void)
{
    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);

    system_init_board();
    actuators_init();
    sensors_init();
    uart_comm_init();

    /* Perifericos del integrador (Etapa 2) */
    leds_init();
    buzzer_init();
    adc_pot_init();
    button_init();
    presence_init();

    app_init();

    tasks_start();

    for (;;) {
    }
}
