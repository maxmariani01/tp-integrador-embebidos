#ifndef PRESENCE_H
#define PRESENCE_H

#include <stdbool.h>

/*
 * Capa de "operador presente" (originalidad sobre la variante Deadman):
 *   - PIR  HC-SR501 en PB1  -> alto = presencia/movimiento detectado.
 *   - Tilt en PB10 (pull-up) -> alto = mando estable (no inclinado/caido).
 * Son entradas lentas, se leen por polling desde la tarea del deadman.
 */

void presence_init(void);
bool presence_pir_active(void); /* true = hay presencia */
bool presence_tilt_ok(void);    /* true = mando estable */

#endif
