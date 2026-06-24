#ifndef BUTTON_H
#define BUTTON_H

#include <stdbool.h>

/*
 * Boton "deadman" en PB0, con resistencia de pull-down EXTERNA (apretar = alto).
 * EXTI0 detecta los flancos; un TIM1 en modo one-shot (20 ms) hace el
 * anti-rebote: cada flanco reinicia la ventana y, cuando vence sin nuevos
 * flancos, se latchea el nivel estable del pin.
 */

void button_init(void);
bool button_is_pressed(void); /* nivel estable, ya filtrado */

#endif
