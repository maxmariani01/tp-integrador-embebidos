# TP Integrador — Blue Pill como interfaz física de robot Unitree

Variante **2 (Deadman switch / hombre muerto)** con una capa de **presencia/integridad
del operador** (PIR + Tilt). La Blue Pill (STM32F103C8T6) habla con un **bridge ROS 2**
por UART usando el protocolo `@LL:TTT:PAYLOAD:CC\n` y reacciona con periféricos físicos
(ADC, PWM, LED, buzzer, EXTI). Reutiliza el protocolo + parser del TP5.

## Concepto

El operador debe mantener un botón apretado **y** estar presente (PIR) **y** el mando
estable (Tilt) para que el robot opere a la velocidad del potenciómetro. Si cualquiera de
las tres condiciones falla → **parada inmediata**. Brillo del LED y tono del buzzer son
proporcionales a la velocidad.

| Condición | Salidas | Tramas |
|---|---|---|
| **Presente** (botón ∧ PIR ∧ tilt) | LED verde brillo ∝ vel., buzzer tono ∝ vel. | `EVT:deadman_ok` (al entrar), `DAT:spd=NN` c/250 ms, `STS:ALIVE` c/1 s |
| **Ausente** | LED rojo fijo, buzzer alarma intermitente | `EVT:deadman` (reenvío c/500 ms hasta `ACK:ok`); +`STS:EMERGENCY` si la causa fue un sensor |

## Conexionado (pinout)

| Función | Pin Blue Pill | Periférico/Timer | Conexión |
|---|---|---|---|
| UART TX | PA9 | USART1 | → RX del adaptador USB-UART |
| UART RX | PA10 | USART1 | → TX del adaptador USB-UART |
| Potenciómetro | PA0 | ADC1_IN0 | cursor del pote (extremos a 3V3 y GND) |
| LED verde (brillo) | PA1 | TIM2_CH2 (PWM) | → 220 Ω → cátodo verde del RGB |
| LED rojo | PB5 | GPIO | → 1 kΩ → cátodo rojo del RGB |
| LED azul (opcional) | PB4 | GPIO | → 220 Ω → cátodo azul del RGB (PB4=NJTRST, liberado por firmware) |
| Buzzer pasivo | PB6 | TIM4_CH1 (PWM) | → buzzer → GND |
| Botón deadman | PB0 | EXTI0 | botón a 3V3, **pull-down externo 10 kΩ** a GND |
| PIR (HC-SR501) | PB1 | GPIO in | OUT del PIR (señal 3.3 V; alimentar a 5 V) |
| Tilt | PB10 | GPIO in (pull-up) | una pata a PB10, otra a GND |
| LED de placa | PC13 | GPIO | heartbeat (ya en la placa) |

RGB de **ánodo común** → ánodo común a **+5V** (a 3.3 V el azul/verde no encienden por su Vf
≈ 3.1–3.3 V). Con ánodo común el pin en BAJO enciende; el firmware ya lo maneja. Es seguro:
la Vf del LED mantiene el pin ≤ 3.3 V. Masa común entre Blue Pill, periféricos y adaptador.
La Blue Pill trabaja a **3.3 V**: el OUT del PIR es 3.3 V (seguro); alimentarlo desde el pin 5V.

## Timers usados

| Timer | Uso | PSC / ARR | Resultado |
|---|---|---|---|
| TIM2 | PWM brillo LED verde | 71 / 999 | 1 kHz, duty 0–1000 |
| TIM4 | PWM tono buzzer | 71 / `1e6/f−1` | frecuencia variable, 50 % duty |
| TIM3 | base de muestreo ADC | 7199 / 99 | IRQ a 100 Hz |
| TIM1 | anti-rebote del botón (one-shot) | 7199 / 199 | ventana de 20 ms |
| SysTick | tick de FreeRTOS | — | 1 kHz |

## Compilar y flashear

```bash
cd codigo/firmware
make flash-deadman   # compila Y flashea la variante (Etapas 3/4)
make flash-demo      # compila Y flashea el modo demo (Etapa 2)
make flash-diag      # variante con diagnóstico de entradas por serie
make                 # solo compila la variante (sin flashear)
make size            # uso de memoria
```

> Usá los atajos `flash-*` (compilan y flashean en un solo paso). Evitá
> `make MODE=demo && make flash`: el segundo `make` no lleva el MODE y recompila a deadman.

> `third_party/` (libopencm3 + FreeRTOS) se referencia del repo base de la cátedra
> `bluepill-freertos-ros2-alumnos` mediante un symlink. Para clonar en otra máquina,
> proveer ese directorio (submódulos).

## Probar en banco (sin robot)

1. Conectar la Blue Pill al adaptador USB-UART (cruzando TX/RX, masa común).
2. Flashear la variante: `make flash`.
3. Correr el emulador del bridge:
   ```bash
   python3 codigo/serial_test/bridge_emulator.py /dev/ttyUSB0
   ```
4. Mantener el botón + estar frente al PIR → el robot "avanza" a la velocidad del pote.
   Soltar el botón, taparse del PIR, o inclinar el tilt → **parada** y alarma local.

## Verificar la lógica del protocolo (sin hardware)

```bash
cd codigo/host_tests && make run     # 18 OK (protocolo) + 20 OK (parser)
```

## Estructura

```
codigo/firmware/    firmware (drivers + FSM de la variante + protocolo/parser del TP5)
codigo/host_tests/  tests de host del protocolo y el parser
codigo/serial_test/ emulador del bridge ROS 2 en Python
informe/            informe técnico final en PDF
presentacion/       presentación del trabajo en PowerPoint
evidencia/          capturas y fotos por etapa
```
