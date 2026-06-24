#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "presence.h"

/*
 * PB1  = PIR  (salida 3.3 V del modulo HC-SR501; pull-down interno).
 * PB10 = Tilt (con pull-up interno). Este modulo deja PB10 en BAJO con el mando
 *        en reposo/derecho, y en ALTO al inclinarlo, asi que:
 *        estable (ok) = nivel BAJO. (Si tu modulo fuera al reves, invertir abajo.)
 */

void presence_init(void)
{
    rcc_periph_clock_enable(RCC_GPIOB);

    gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO1);
    gpio_clear(GPIOB, GPIO1);  /* pull-down: sin PIR conectado/no activo => 0 */
    gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO10);
    gpio_set(GPIOB, GPIO10); /* selecciona pull-up en PB10 */
}

bool presence_pir_active(void)
{
    return gpio_get(GPIOB, GPIO1) != 0U;
}

bool presence_tilt_ok(void)
{
    return gpio_get(GPIOB, GPIO10) == 0U; /* reposo = bajo = estable */
}
