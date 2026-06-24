#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "deadman.h"
#include "../drivers/adc.h"
#include "../drivers/leds.h"
#include "../drivers/buzzer.h"
#include "../drivers/button.h"
#include "../drivers/presence.h"
#include "../protocol/protocol.h"
#include "../config/app_config.h"

/* ACK:ok pendiente, seteado desde la tarea de aplicacion (otra tarea). */
static volatile uint8_t g_ack_ok = 0U;

void deadman_notify_ack(const char *payload)
{
    if ((payload != NULL) && (strcmp(payload, "ok") == 0)) {
        g_ack_ok = 1U;
    }
}

static bool take_ack(void)
{
    if (g_ack_ok != 0U) {
        g_ack_ok = 0U;
        return true;
    }
    return false;
}

static void send_msg(QueueHandle_t tx, protocol_type_t type, const char *payload)
{
    protocol_message_t message;
    if (protocol_message_set(&message, type, payload)) {
        (void) xQueueSend(tx, &message, 0);
    }
}

static void send_spd(QueueHandle_t tx, uint8_t spd)
{
    char payload[PROTOCOL_MAX_PAYLOAD_LENGTH + 1U];
    /* Ancho fijo de 2 digitos con relleno de ceros: spd=07, spd=100 */
    snprintf(payload, sizeof(payload), "spd=%02u", (unsigned) spd);
    send_msg(tx, PROTOCOL_TYPE_DAT, payload);
}

static bool stable_bool_update(bool raw, bool stable, uint8_t *change_count)
{
    if (raw == stable) {
        *change_count = 0U;
        return stable;
    }

    if (*change_count < DEADMAN_INPUT_STABLE_SAMPLES) {
        (*change_count)++;
    }
    if (*change_count >= DEADMAN_INPUT_STABLE_SAMPLES) {
        stable = raw;
        *change_count = 0U;
    }
    return stable;
}

void deadman_task(void *args)
{
    QueueHandle_t tx = (QueueHandle_t) args;
    TickType_t last = xTaskGetTickCount();
    bool present_prev = false;
    bool ack_pending = false;
    bool beep_on = false;
    bool armed_once = false;   /* true tras la primera vez que hubo operador */
    uint32_t t_spd = 0U;
    uint32_t t_alive = 0U;
    uint32_t t_ack = 0U;
    uint32_t t_beep = 0U;
    bool muted = false;          /* doble pulsacion rapida silencia el buzzer */
    bool btn_prev = false;       /* para detectar el flanco de pulsacion */
    TickType_t last_rise = 0;    /* tick de la ultima pulsacion */
    bool pir_stable = presence_pir_active();
    bool tilt_stable = presence_tilt_ok();
    uint8_t pir_change_count = 0U;
    uint8_t tilt_change_count = 0U;
#if DEADMAN_DIAG
    uint32_t t_diag = 0U;
#endif

    /* Estado inicial: detenido (operador ausente) */
    leds_green_set_brightness(0U);
    leds_red_set(true);
    buzzer_off();

    for (;;) {
        bool pressed = button_is_pressed();
        bool pir_raw = presence_pir_active();
        bool tilt_raw = presence_tilt_ok();
        bool pir;
        bool tilt_ok;
        bool present;
        uint8_t spd = adc_pot_get_speed_pct();

        pir_stable = stable_bool_update(pir_raw, pir_stable, &pir_change_count);
        tilt_stable = stable_bool_update(tilt_raw, tilt_stable, &tilt_change_count);
        pir = pir_stable;
        tilt_ok = tilt_stable;
        present = pressed && pir && tilt_ok;

        /* Doble pulsacion rapida (dos flancos dentro de la ventana) -> mute */
        if (pressed && !btn_prev) {
            TickType_t now = xTaskGetTickCount();
            if ((now - last_rise) <= pdMS_TO_TICKS(DEADMAN_DOUBLE_TAP_MS)) {
                muted = !muted;
            }
            last_rise = now;
        }
        btn_prev = pressed;

#if DEADMAN_DIAG
        /* Diagnostico: entradas crudas y filtradas cada 300 ms */
        t_diag += DEADMAN_TASK_PERIOD_MS;
        if (t_diag >= 300U) {
            char diag[PROTOCOL_MAX_PAYLOAD_LENGTH + 1U];
            t_diag = 0U;
            snprintf(diag, sizeof(diag), "b=%u,pr=%u,ps=%u,tr=%u,ts=%u",
                     (unsigned) pressed, (unsigned) pir_raw, (unsigned) pir,
                     (unsigned) tilt_raw, (unsigned) tilt_ok);
            send_msg(tx, PROTOCOL_TYPE_DAT, diag);
        }
#endif

        /* --- Transiciones --- */
        if (present && !present_prev) {
            /* Ausente -> Presente: el robot puede operar */
            (void) take_ack();          /* descarta cualquier ACK viejo */
            ack_pending = false;
            armed_once = true;
            t_spd = 0U;
            t_alive = 0U;
            send_msg(tx, PROTOCOL_TYPE_EVT, "deadman_ok");
        } else if (!present && present_prev) {
            /* Presente -> Ausente: parada inmediata */
            bool sensor_cause = pressed; /* sigue apretado => fallo PIR/tilt */
            send_msg(tx, PROTOCOL_TYPE_EVT, "deadman");
            if (sensor_cause) {
                /* La causa fue un sensor: lo hacemos visible con STS:EMERGENCY */
                send_msg(tx, PROTOCOL_TYPE_STS, "EMERGENCY");
            }
            ack_pending = true;
            t_ack = 0U;
            t_beep = 0U;
            beep_on = false;
        }
        present_prev = present;

        /* --- Salidas y periodicos segun el estado --- */
        if (present) {
            uint16_t freq;
            leds_red_set(false);
            leds_blue_set(false);   /* azul apagado al operar: el verde se ve limpio */
            leds_green_set_brightness((uint16_t) spd * 10U);

            freq = (uint16_t) (BUZZER_FREQ_MIN_HZ +
                ((uint32_t) (BUZZER_FREQ_MAX_HZ - BUZZER_FREQ_MIN_HZ) * spd) / 100U);
            if (muted) {
                buzzer_off();
            } else {
                buzzer_set_tone(freq);
            }

            t_spd += DEADMAN_TASK_PERIOD_MS;
            if (t_spd >= DEADMAN_SPD_PERIOD_MS) {
                t_spd = 0U;
                send_spd(tx, spd);
            }

            t_alive += DEADMAN_TASK_PERIOD_MS;
            if (t_alive >= DEADMAN_ALIVE_PERIOD_MS) {
                t_alive = 0U;
                send_msg(tx, PROTOCOL_TYPE_STS, "ALIVE");
            }
        } else {
            leds_green_set_brightness(0U);
            leds_red_set(true);
            leds_blue_set(pir);   /* detenido: azul = el PIR te detecta (listo para reenganchar) */

            /* Pitido de alarma intermitente (~4 Hz), solo si ya hubo operador
             * y el buzzer no esta silenciado. En el arranque queda callado. */
            if (armed_once && !muted) {
                t_beep += DEADMAN_TASK_PERIOD_MS;
                if (t_beep >= 125U) {
                    t_beep = 0U;
                    beep_on = !beep_on;
                    if (beep_on) {
                        buzzer_set_tone(BUZZER_ALARM_HZ);
                    } else {
                        buzzer_off();
                    }
                }
            } else {
                buzzer_off();
            }

            /* Reenvio de EVT:deadman cada 500 ms hasta recibir ACK:ok */
            if (ack_pending) {
                if (take_ack()) {
                    ack_pending = false;
                } else {
                    t_ack += DEADMAN_TASK_PERIOD_MS;
                    if (t_ack >= DEADMAN_ACK_TIMEOUT_MS) {
                        t_ack = 0U;
                        send_msg(tx, PROTOCOL_TYPE_EVT, "deadman");
                    }
                }
            }
        }

        vTaskDelayUntil(&last, pdMS_TO_TICKS(DEADMAN_TASK_PERIOD_MS));
    }
}
