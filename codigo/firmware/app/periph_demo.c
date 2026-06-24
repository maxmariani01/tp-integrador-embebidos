#include <stdbool.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "periph_demo.h"
#include "../drivers/adc.h"
#include "../drivers/leds.h"
#include "../drivers/buzzer.h"
#include "../drivers/button.h"
#include "../drivers/presence.h"
#include "../protocol/protocol.h"
#include "../config/app_config.h"

void periph_demo_task(void *args)
{
    QueueHandle_t tx_queue = (QueueHandle_t) args;
    TickType_t last = xTaskGetTickCount();
    protocol_message_t message;
    char payload[PROTOCOL_MAX_PAYLOAD_LENGTH + 1U];

    for (;;) {
        uint16_t raw = adc_pot_get_raw();
        uint8_t spd = adc_pot_get_speed_pct();
        bool pressed = button_is_pressed();

        /* Brillo del LED verde proporcional al pote (0..1000) */
        leds_green_set_brightness((uint16_t) spd * 10U);

        /* Indicadores: rojo = boton apretado, azul = PIR activo */
        leds_red_set(pressed);
        leds_blue_set(presence_pir_active());

        /* Buzzer: tono proporcional mientras el boton este apretado */
        if (pressed) {
            uint16_t freq = (uint16_t) (BUZZER_FREQ_MIN_HZ +
                ((uint32_t) (BUZZER_FREQ_MAX_HZ - BUZZER_FREQ_MIN_HZ) * spd) / 100U);
            buzzer_set_tone(freq);
        } else {
            buzzer_off();
        }

        /* Telemetria DAT:adc=NNNN (4 digitos, relleno con ceros) */
        snprintf(payload, sizeof(payload), "adc=%04u", (unsigned) raw);
        if (protocol_message_set(&message, PROTOCOL_TYPE_DAT, payload)) {
            (void) xQueueSend(tx_queue, &message, 0);
        }

        vTaskDelayUntil(&last, pdMS_TO_TICKS(250));
    }
}
