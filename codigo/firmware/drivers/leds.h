#ifndef LEDS_H
#define LEDS_H

#include <stdbool.h>
#include <stdint.h>

/*
 * LED RGB de ANODO COMUN:
 *   - Verde  -> PA1 (TIM2_CH2, PWM de brillo, 0..1000 por mil)
 *   - Rojo   -> PB5 (GPIO on/off)
 *   - Azul   -> PB4 (GPIO on/off)
 * Cada color va por su resistor de 220 ohm al pin; anodo comun a +3V3.
 * Ojo: con anodo comun el pin en BAJO enciende (logica invertida).
 */

void leds_init(void);
void leds_green_set_brightness(uint16_t permille); /* 0..1000 (0..100.0 %) */
void leds_red_set(bool on);
void leds_blue_set(bool on);

#endif
