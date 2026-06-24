#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "system_init.h"

void system_init_board(void)
{
    rcc_periph_clock_enable(RCC_AFIO);

    /*
     * PB4 es NJTRST (pin de JTAG) y por defecto NO funciona como GPIO.
     * Liberamos solo NJTRST manteniendo SWD activo (el ST-Link sigue
     * flasheando por SWDIO/SWCLK). Necesario para usar PB4 como salida del
     * LED azul.
     */
    gpio_primary_remap(AFIO_MAPR_SWJ_CFG_FULL_SWJ_NO_JNTRST, 0);
}
