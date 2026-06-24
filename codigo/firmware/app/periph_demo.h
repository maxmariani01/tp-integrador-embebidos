#ifndef PERIPH_DEMO_H
#define PERIPH_DEMO_H

#include "FreeRTOS.h"
#include "queue.h"

/*
 * Tarea de verificacion de la Etapa 2 (nucleo de perifericos).
 * Recibe la cola de TX (QueueHandle_t) como argumento.
 * Mapea el pote a brillo de LED y a tono de buzzer, refleja boton/PIR en los
 * LEDs y emite DAT:adc=NNNN periodico. No implementa la variante todavia.
 */
void periph_demo_task(void *args);

#endif
