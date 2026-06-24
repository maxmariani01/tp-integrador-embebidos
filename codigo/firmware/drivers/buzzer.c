#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>

#include "buzzer.h"
#include "../config/app_config.h"

/*
 * TIM4_CH1 en PB6. Cuenta a 1 MHz (PSC=71).
 * Para un tono f: periodo ARR = 1_000_000 / f - 1, y CCR = (ARR+1)/2 -> 50 %.
 * Silencio: CCR = 0 (con PWM1 la salida queda en bajo todo el periodo).
 */

void buzzer_init(void)
{
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_TIM4);

    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO6);

    timer_set_mode(TIM4, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_set_prescaler(TIM4, BUZZER_TIM_PRESCALER);
    timer_set_period(TIM4, 999U); /* valor inicial, se reemplaza en set_tone */

    timer_set_oc_mode(TIM4, TIM_OC1, TIM_OCM_PWM1);
    timer_set_oc_value(TIM4, TIM_OC1, 0U); /* arranca en silencio */
    timer_enable_oc_output(TIM4, TIM_OC1);

    timer_generate_event(TIM4, TIM_EGR_UG);
    timer_enable_counter(TIM4);
}

void buzzer_set_tone(uint16_t freq_hz)
{
    uint32_t arr;

    if (freq_hz == 0U) {
        buzzer_off();
        return;
    }

    arr = 1000000U / (uint32_t) freq_hz;
    if (arr == 0U) {
        arr = 1U;
    }
    arr -= 1U;

    timer_set_period(TIM4, arr);
    timer_set_oc_value(TIM4, TIM_OC1, (arr + 1U) / 2U); /* 50 % */
}

void buzzer_off(void)
{
    timer_set_oc_value(TIM4, TIM_OC1, 0U);
}
