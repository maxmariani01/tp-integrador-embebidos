# Guía de pruebas en hardware + checklist de evidencia

## 0. Materiales

Blue Pill + ST-Link V2, adaptador USB-UART CH340 (3.3 V), protoboard, LED RGB (ánodo común),
1× 1 kΩ, 2× 220 Ω, 1× 10 kΩ, buzzer **pasivo**, pulsador, potenciómetro 10 kΩ, módulo PIR HC-SR501,
módulo Tilt, cables.

## 1. Cableado (protoboard)

Seguí la tabla del README. Resumen rápido:

| Pin | A |
|---|---|
| PA9 → RX adaptador, PA10 → TX adaptador, GND ↔ GND | UART (cruzado) |
| PA0 | cursor del pote (extremos a 3V3 y GND) |
| PA1 →220Ω→ verde, PB5 →220Ω→ rojo, PB4 →220Ω→ azul | RGB **ánodo común → +5V** |
| PB6 | buzzer (+), otra pata a GND |
| PB0 | pulsador a 3V3 **y** 10kΩ de PB0 a GND (pull-down) |
| PB1 | OUT del PIR (VCC del PIR a **5V**, GND común) |
| PB10 | DO del tilt (a GND si es switch de 2 patas; usa pull-up interno) |

> **Masa común** entre todo. El PIR se alimenta a 5 V pero su OUT es 3.3 V (seguro).

## 2. Flashear

```bash
cd ~/BLUEPILL/tp-integrador-embebidos-mariani/codigo/firmware
make flash-demo      # Etapa 2 (demo de periféricos): compila y flashea
# ...probar Etapa 2...
make flash-deadman   # Etapa 3/4 (variante deadman): compila y flashea
```

Puerto serie: confirmá con `ls /dev/ttyUSB*`. Si da "permission denied":
`sudo chmod 666 /dev/ttyUSB0` (o sumar tu usuario al grupo `dialout`).

## 3. Etapa 2 — demo de periféricos  (`make MODE=demo`)

Monitor serie: `python3 ../serial_test/bridge_emulator.py /dev/ttyUSB0`
(o `picocom -b 115200 /dev/ttyUSB0`).

- Girá el **pote**: el LED verde cambia de brillo y verás `DAT:adc=NNNN` cambiar (0000–4095).
- Apretá el **botón**: el LED rojo se enciende y el buzzer suena con tono ∝ pote.
- Movete frente al **PIR**: el LED azul se enciende.

**Evidencia E2:** captura serie con ≥10 valores `adc=NNNN` distintos (girando el pote) + foto/video
del LED variando el brillo y el buzzer sonando.

## 4. Etapa 3/4 — variante deadman  (`make`)

Monitor: `python3 ../serial_test/bridge_emulator.py /dev/ttyUSB0`

1. **Mantené el botón + ponete frente al PIR + mando estable** → estado *presente*:
   - LED verde con brillo ∝ pote, buzzer con tono ∝ pote.
   - Serie: `EVT:deadman_ok` una vez, luego `DAT:spd=NN` c/250 ms y `STS:ALIVE` c/1 s.
   - El emulador muestra el tópico ROS 2 de cada trama y la "reacción del robot".
2. **Soltá el botón** → *ausente* por botón:
   - LED rojo fijo, buzzer alarma intermitente.
   - Serie: `EVT:deadman`; el emulador responde `ACK:ok` y para el reenvío.
3. **Con el botón apretado, tapate del PIR** (o inclina el tilt) → *ausente por sensor*:
   - Serie: `EVT:deadman` **+ `STS:EMERGENCY`** (la causa fue un sensor).

> El PIR tarda ~30–60 s en estabilizar al encender y mantiene la salida alta unos
> segundos tras detectar movimiento (es normal). Si el **tilt** quedara invertido
> (para con el mando derecho), negá el retorno de `presence_tilt_ok()` y recompilá.

## 5. Escenarios de falla (Etapa 4) — capturá al menos uno

| Escenario | Cómo | Qué pasa |
|---|---|---|
| Soltar botón | soltá el deadman | `EVT:deadman`, parada, alarma |
| Falla de sensor | apretado pero tapás el PIR / inclinás el tilt | `EVT:deadman` + `STS:EMERGENCY` |
| Timeout de ACK | cortá el emulador y soltá el botón | la Pill **reenvía** `EVT:deadman` c/500 ms |
| UART desconectado | desenchufá el cable de datos | el bridge deja de recibir `ALIVE` (robot pararía) |
| Trama corrupta | ver abajo | el parser **se resincroniza** y sigue |

Mandar una trama corrupta desde la PC (para ver resync):
```bash
python3 -c "import serial; serial.Serial('/dev/ttyUSB0',115200).write(b'basura@@xx\n')"
```

## 6. Checklist de evidencia para el informe

- [ ] **E1** `cd ../host_tests && make run` → captura "18 OK / 20 OK".
- [ ] **E1** captura serie con ≥5 tramas y sus respuestas (sirve la del deadman con ACK:ok).
- [ ] **E2** captura serie con ≥10 lecturas `adc=NNNN` variando el pote.
- [ ] **E2** foto/video: LED con brillo variable + buzzer sonando.
- [ ] **E3** captura serie ≥20 s de operación continua (deadman_ok / spd / ALIVE / deadman).
- [ ] **E3** foto del montaje completo (Blue Pill + protoboard + periféricos).
- [ ] **E4** captura del emulador mostrando tramas + tópicos ROS 2 + el `ACK:ok`.
- [ ] **E4** captura de un escenario de falla (recomendado: falla de sensor con `STS:EMERGENCY`).
- [ ] (opcional) `make size` (uso de memoria), y video del robot si hay acceso al lab.

Guardá todo en `evidencia/etapa2|etapa3|etapa4/`. Con eso armamos el informe.
