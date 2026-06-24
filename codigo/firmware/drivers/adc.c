#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/cm3/nvic.h>

#include "adc.h"
#include "../config/app_config.h"

/* Ultima muestra suavizada con un filtro EMA (alpha = 1/8). */
static volatile uint16_t g_adc_ema = 0U;

static uint16_t adc_read_blocking(void)
{
    adc_start_conversion_direct(ADC1);
    while (!adc_eoc(ADC1)) {
    }
    return (uint16_t) adc_read_regular(ADC1);
}

void adc_pot_init(void)
{
    uint8_t channel = 0U; /* PA0 = canal 0 */

    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_ADC1);
    rcc_periph_clock_enable(RCC_TIM3);

    /* El ADC no puede pasar de 14 MHz: 72 MHz / 6 = 12 MHz */
    rcc_set_adcpre(RCC_CFGR_ADCPRE_PCLK2_DIV6);

    /* PA0 como entrada analogica */
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO0);

    adc_power_off(ADC1);
    adc_disable_scan_mode(ADC1);
    adc_set_single_conversion_mode(ADC1);
    adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_28DOT5CYC);
    adc_set_regular_sequence(ADC1, 1, &channel);
    adc_power_on(ADC1);

    /* Espera de estabilizacion y calibracion */
    for (volatile int i = 0; i < 10000; i++) {
        __asm__("nop");
    }
    adc_reset_calibration(ADC1);
    adc_calibrate(ADC1);

    /* Semilla del filtro con una primera lectura real */
    g_adc_ema = adc_read_blocking();

    /* TIM3: 10 kHz de cuenta, periodo 100 -> update a 100 Hz */
    timer_set_mode(TIM3, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_set_prescaler(TIM3, ADC_TIM_PRESCALER);
    timer_set_period(TIM3, ADC_TIM_PERIOD);
    timer_generate_event(TIM3, TIM_EGR_UG); /* precarga PSC/ARR */
    timer_clear_flag(TIM3, TIM_SR_UIF);
    timer_enable_irq(TIM3, TIM_DIER_UIE);

    nvic_set_priority(NVIC_TIM3_IRQ, 0x70);
    nvic_enable_irq(NVIC_TIM3_IRQ);

    timer_enable_counter(TIM3);
}

uint16_t adc_pot_get_raw(void)
{
    return g_adc_ema;
}

uint8_t adc_pot_get_speed_pct(void)
{
    uint32_t v = (uint32_t) g_adc_ema * 100U / ADC_MAX_COUNT;
    if (v > 100U) {
        v = 100U;
    }
    return (uint8_t) v;
}

/* Cada 10 ms: una conversion y actualizacion del promedio. Sin API FreeRTOS. */
void tim3_isr(void)
{
    if (timer_get_flag(TIM3, TIM_SR_UIF)) {
        uint16_t raw;
        timer_clear_flag(TIM3, TIM_SR_UIF);
        raw = adc_read_blocking();
        g_adc_ema = (uint16_t)(g_adc_ema - (g_adc_ema >> 3) + (raw >> 3));
    }
}
