#ifndef ADC_POT_H
#define ADC_POT_H

#include <stdint.h>

/*
 * Potenciometro en PA0 = ADC1_IN0.
 * TIM3 corre a 100 Hz como "base de tiempo": en cada update su ISR dispara
 * una conversion y la promedia (filtro EMA) para suavizar el ruido del pote.
 */

void adc_pot_init(void);
uint16_t adc_pot_get_raw(void);       /* 0..4095 (suavizado) */
uint8_t adc_pot_get_speed_pct(void);  /* 0..100 % mapeado del ADC */

#endif
