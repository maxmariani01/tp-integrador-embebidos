#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>

#include "leds.h"
#include "../config/app_config.h"

/*
 * Verde en PA1 = TIM2_CH2 con PWM a 1 kHz (frecuencia fija) y duty variable.
 * A 1 kHz el ojo no ve parpadeo y el promedio del duty se percibe como brillo.
 * Rojo (PB5) y azul (PB4) son salidas digitales simples.
 *
 * LED RGB de ANODO COMUN: el pin en BAJO enciende. Por eso el verde usa
 * polaridad de salida en bajo (CCR=duty sigue significando brillo) y el rojo/azul
 * encienden con gpio_clear() y apagan con gpio_set().
 *
 * El rojo queda en open-drain porque el anodo comun va a 5 V: apagado = alta
 * impedancia, evitando que el rojo conduzca debilmente contra el nivel alto de
 * 3.3 V del GPIO.
 */

void leds_init(void)
{
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_TIM2);

    /* PA1 como salida de funcion alternativa (canal del timer) */
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO1);

    /* Rojo: open-drain. Azul: push-pull. Apagados = latch alto. */
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_2_MHZ,
                  GPIO_CNF_OUTPUT_OPENDRAIN, GPIO5);
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_2_MHZ,
                  GPIO_CNF_OUTPUT_PUSHPULL, GPIO4);
    gpio_set(GPIOB, GPIO5 | GPIO4);

    /* TIM2: contador ascendente, 1 MHz de cuenta, periodo 1000 -> 1 kHz */
    timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_set_prescaler(TIM2, LED_PWM_PRESCALER);
    timer_set_period(TIM2, LED_PWM_PERIOD);

    /* Canal 2 en PWM1 con polaridad BAJA (anodo comun): pin en bajo = encendido */
    timer_set_oc_mode(TIM2, TIM_OC2, TIM_OCM_PWM1);
    timer_set_oc_polarity_low(TIM2, TIM_OC2);
    timer_set_oc_value(TIM2, TIM_OC2, 0U);
    timer_enable_oc_output(TIM2, TIM_OC2);

    /* Precarga PSC/ARR antes de arrancar (primer periodo correcto) */
    timer_generate_event(TIM2, TIM_EGR_UG);
    timer_enable_counter(TIM2);
}

void leds_green_set_brightness(uint16_t permille)
{
    if (permille > LED_PWM_DUTY_MAX) {
        permille = LED_PWM_DUTY_MAX;
    }
    timer_set_oc_value(TIM2, TIM_OC2, permille);
}

/* Anodo comun: encender = pin en BAJO (gpio_clear), apagar = pin en ALTO */
void leds_red_set(bool on)
{
    if (on) {
        gpio_clear(GPIOB, GPIO5);
    } else {
        gpio_set(GPIOB, GPIO5);
    }
}

void leds_blue_set(bool on)
{
    if (on) {
        gpio_clear(GPIOB, GPIO4);
    } else {
        gpio_set(GPIOB, GPIO4);
    }
}
