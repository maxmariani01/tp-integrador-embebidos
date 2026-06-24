#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#define UART_BAUDRATE                 115200U

#define UART_RX_ISR_QUEUE_LENGTH      64U
#define PARSER_INPUT_QUEUE_LENGTH     64U
#define APP_MESSAGE_QUEUE_LENGTH      8U
#define UART_TX_QUEUE_LENGTH          8U
#define ACTUATOR_QUEUE_LENGTH         8U

#define TELEMETRY_PERIOD_MS           1000U
#define STATUS_PERIOD_MS              5000U

#define PROTOCOL_MAX_PAYLOAD_LENGTH   48U
#define PROTOCOL_MAX_FRAME_LENGTH     64U

/* ===== Integrador: nucleo de perifericos y variante Deadman ===== */

/* Modo de build: demo de perifericos (Etapa 2) vs variante deadman (Etapa 3/4) */
#define APP_BUILD_MODE_PERIPH_DEMO    1
#define APP_BUILD_MODE_DEADMAN        2
#ifndef APP_BUILD_MODE
#define APP_BUILD_MODE                APP_BUILD_MODE_DEADMAN
#endif

/* Diagnostico: si =1, el deadman emite DAT:b=_,p=_,t=_ con el estado crudo de
 * boton/PIR/tilt cada 300 ms. Se activa con "make DIAG=1". Default off. */
#ifndef DEADMAN_DIAG
#define DEADMAN_DIAG                  0
#endif

/* LED verde PWM (TIM2_CH2 / PA1): 72MHz/(71+1)=1MHz, /(999+1)=1kHz, duty 0..1000 */
#define LED_PWM_PRESCALER             71U
#define LED_PWM_PERIOD                999U
#define LED_PWM_DUTY_MAX              1000U

/* Buzzer PWM (TIM4_CH1 / PB6): cuenta a 1 MHz; ARR = 1e6/f - 1 */
#define BUZZER_TIM_PRESCALER          71U
#define BUZZER_FREQ_MIN_HZ            1000U
#define BUZZER_FREQ_MAX_HZ            4000U
#define BUZZER_ALARM_HZ               2000U

/* ADC pote (PA0 / ADC1_IN0), base TIM3: 72MHz/7200=10kHz, /100=100Hz */
#define ADC_TIM_PRESCALER             7199U
#define ADC_TIM_PERIOD                99U
#define ADC_MAX_COUNT                 4095U

/* Anti-rebote del boton (TIM1 one-shot): 10kHz, 200 cuentas = 20 ms */
#define BUTTON_DEBOUNCE_TIM_PRESCALER 7199U
#define BUTTON_DEBOUNCE_TIM_PERIOD    199U

/* Temporizacion de la variante Deadman */
#define DEADMAN_TASK_PERIOD_MS        20U
#define DEADMAN_SPD_PERIOD_MS         250U
#define DEADMAN_ALIVE_PERIOD_MS       1000U
#define DEADMAN_ACK_TIMEOUT_MS        500U
#define DEADMAN_DOUBLE_TAP_MS         400U   /* ventana para la doble pulsacion (mute) */
#define DEADMAN_INPUT_STABLE_SAMPLES  3U     /* 3 * 20 ms: filtra ruido breve de PIR/tilt */

#endif
