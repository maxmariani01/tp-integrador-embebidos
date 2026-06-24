#ifndef DEADMAN_H
#define DEADMAN_H

#include "FreeRTOS.h"
#include "queue.h"

/*
 * Variante 2: Deadman switch / hombre muerto, con capa de presencia.
 *
 * "Operador presente" = boton apretado AND PIR con presencia AND tilt estable.
 * Presente  -> robot opera a la velocidad del pote (DAT:spd), LED/buzzer activos.
 * Ausente   -> parada (EVT:deadman, reenvio hasta ACK:ok), alarma local.
 *
 * deadman_task: tarea de la FSM; recibe la cola de TX como argumento.
 * deadman_notify_ack: la llama la capa de aplicacion al recibir un ACK.
 */

void deadman_task(void *args);
void deadman_notify_ack(const char *payload);

#endif
