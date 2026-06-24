#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>

/*
 * Buzzer PASIVO en PB6 = TIM4_CH1.
 * Un buzzer pasivo necesita una senal cuadrada: la frecuencia define el tono.
 * Variamos el periodo (ARR) del timer para cambiar la nota; duty fijo ~50 %.
 */

void buzzer_init(void);
void buzzer_set_tone(uint16_t freq_hz); /* 0 = silencio */
void buzzer_off(void);

#endif
