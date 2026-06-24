#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/cm3/nvic.h>

#include "button.h"
#include "../config/app_config.h"

static volatile uint8_t g_pressed = 0U;

void button_init(void)
{
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_TIM1);
    /* RCC_AFIO ya se habilito en system_init_board() (necesario para EXTI) */

    /* PB0 con pull-down interno como respaldo del pull-down externo de la placa. */
    gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO0);
    gpio_clear(GPIOB, GPIO0);
    g_pressed = (gpio_get(GPIOB, GPIO0) != 0U) ? 1U : 0U;

    /* TIM1 como base de tiempo one-shot de 20 ms (10 kHz, periodo 200) */
    timer_set_mode(TIM1, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_set_prescaler(TIM1, BUTTON_DEBOUNCE_TIM_PRESCALER);
    timer_set_period(TIM1, BUTTON_DEBOUNCE_TIM_PERIOD);
    timer_one_shot_mode(TIM1);
    timer_generate_event(TIM1, TIM_EGR_UG); /* precarga PSC/ARR, no arranca el contador */
    timer_clear_flag(TIM1, TIM_SR_UIF);
    timer_enable_irq(TIM1, TIM_DIER_UIE);
    nvic_set_priority(NVIC_TIM1_UP_IRQ, 0x70);
    nvic_enable_irq(NVIC_TIM1_UP_IRQ);

    /* EXTI0 sobre PB0, ambos flancos (apretar y soltar) */
    exti_select_source(EXTI0, GPIOB);
    exti_set_trigger(EXTI0, EXTI_TRIGGER_BOTH);
    exti_enable_request(EXTI0);
    nvic_set_priority(NVIC_EXTI0_IRQ, 0x60);
    nvic_enable_irq(NVIC_EXTI0_IRQ);
}

bool button_is_pressed(void)
{
    return g_pressed != 0U;
}

/* Cada flanco reinicia la ventana de anti-rebote */
void exti0_isr(void)
{
    exti_reset_request(EXTI0);
    timer_set_counter(TIM1, 0U);
    timer_enable_counter(TIM1);
}

/* Vencio la ventana: el nivel actual del pin es el estado estable */
void tim1_up_isr(void)
{
    if (timer_get_flag(TIM1, TIM_SR_UIF)) {
        timer_clear_flag(TIM1, TIM_SR_UIF);
        g_pressed = (gpio_get(GPIOB, GPIO0) != 0U) ? 1U : 0U;
    }
}
