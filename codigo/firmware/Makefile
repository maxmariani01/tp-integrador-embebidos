# Proyecto base STM32F103C8T6 con libopencm3 + FreeRTOS
PROJECT = main
TARGET = $(PROJECT).elf
BUILD_DIR = bin
APP_CONFIG_HEADER = config/app_config.h

# Rutas base
THIRD_PARTY_DIR ?= third_party
COMMON_DIR ?= $(THIRD_PARTY_DIR)/common
LIBOPENCM3_DIR ?= $(THIRD_PARTY_DIR)/libopencm3
FREERTOS_DIR ?= $(THIRD_PARTY_DIR)/FreeRTOS-Kernel

HEAP_4_SRC := $(FREERTOS_DIR)/portable/MemMang/heap_4.c
ifeq ($(wildcard $(HEAP_4_SRC)),)
HEAP_4_SRC := $(FREERTOS_DIR)/heap_4.c
endif

MCU = cortex-m3
CDEFS = -DSTM32F1

# Modo de build: "make" / "make MODE=deadman" (variante, Etapa 3/4)
#                "make MODE=demo" (nucleo de perifericos, Etapa 2).
MODE ?= deadman
ifeq ($(MODE),demo)
CDEFS += -DAPP_BUILD_MODE=1
else
CDEFS += -DAPP_BUILD_MODE=2
endif

# Diagnostico opcional: "make DIAG=1" emite el estado crudo de las entradas.
ifeq ($(DIAG),1)
CDEFS += -DDEADMAN_DIAG=1
endif

# Compilador
CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy

INCLUDES  = -I$(LIBOPENCM3_DIR)/include
INCLUDES += -I$(COMMON_DIR)
INCLUDES += -I$(FREERTOS_DIR)
INCLUDES += -I$(FREERTOS_DIR)/include
INCLUDES += -I$(FREERTOS_DIR)/portable/GCC/ARM_CM3
INCLUDES += -I./config
INCLUDES += -I./app
INCLUDES += -I./drivers
INCLUDES += -I./platform
INCLUDES += -I./protocol
INCLUDES += -I./

# Flags e linker
CFLAGS = -mcpu=$(MCU) -mthumb -Wall -Wextra -O0 -g -ffunction-sections -fdata-sections $(INCLUDES) $(CDEFS) -nostdlib
LDSCRIPT = $(COMMON_DIR)/linker.ld
LDFLAGS = -nostartfiles --specs=nano.specs -Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/$(PROJECT).map -T$(LDSCRIPT)
LDLIBS = -L$(LIBOPENCM3_DIR)/lib -lopencm3_stm32f1 -lc -lgcc -lnosys

FREERTOS_SRC = \
    $(FREERTOS_DIR)/list.c \
    $(FREERTOS_DIR)/queue.c \
    $(FREERTOS_DIR)/tasks.c \
    $(FREERTOS_DIR)/portable/GCC/ARM_CM3/port.c \
    $(HEAP_4_SRC)

LOCAL_SRC = \
    main.c \
    platform/system_init.c \
    app/tasks.c \
    drivers/uart_comm.c \
    drivers/leds.c \
    drivers/buzzer.c \
    drivers/adc.c \
    drivers/button.c \
    drivers/presence.c \
    protocol/protocol.c \
    protocol/parser.c \
    app/app.c \
    app/sensors.c \
    app/actuators.c \
    app/periph_demo.c \
    app/deadman.c

LOCAL_OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(LOCAL_SRC))
FREERTOS_OBJ = $(patsubst $(FREERTOS_DIR)/%.c,$(BUILD_DIR)/freertos/%.o,$(FREERTOS_SRC))
OBJ = $(LOCAL_OBJ) $(FREERTOS_OBJ)

# El sello de MODE (abajo) define un target antes de 'all'; fijamos el goal
# por defecto para que "make" siga compilando todo y no solo el sello.
.DEFAULT_GOAL := all

# Sello de MODE: make solo mira timestamps, no los flags -D. Sin esto, cambiar
# de demo a deadman no recompila y se flashea el binario viejo. El sello cambia
# de nombre con el MODE, forzando a recompilar los objetos locales.
MODE_STAMP := $(BUILD_DIR)/.build-$(MODE)-$(DIAG)
$(MODE_STAMP):
	@mkdir -p $(BUILD_DIR)
	@rm -f $(BUILD_DIR)/.build-*
	@touch $@
$(LOCAL_OBJ): $(MODE_STAMP)

.PHONY: all clean flash size help check-deps
.PHONY: openocd gdb flash-demo flash-deadman flash-diag

all: check-deps $(LIBOPENCM3_DIR)/lib/libopencm3_stm32f1.a $(BUILD_DIR)/$(TARGET)

check-deps:
	@test -f "$(COMMON_DIR)/linker.ld" || (echo "Falta $(COMMON_DIR)/linker.ld" && exit 1)
	@test -f "$(LIBOPENCM3_DIR)/include/libopencm3/stm32/usart.h" || (echo "Falta libopencm3 en $(LIBOPENCM3_DIR)" && exit 1)
	@test -f "$(FREERTOS_DIR)/include/FreeRTOS.h" || (echo "Falta FreeRTOS-Kernel en $(FREERTOS_DIR)" && exit 1)

$(OBJ): $(APP_CONFIG_HEADER)

$(BUILD_DIR)/$(TARGET): $(OBJ) $(LIBOPENCM3_DIR)/lib/libopencm3_stm32f1.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)
	$(OBJCOPY) -O ihex $@ $(BUILD_DIR)/$(PROJECT).hex
	$(OBJCOPY) -O binary $@ $(BUILD_DIR)/$(PROJECT).bin

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/freertos/%.o: $(FREERTOS_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(LIBOPENCM3_DIR)/lib/libopencm3_stm32f1.a:
	$(MAKE) -C $(LIBOPENCM3_DIR)

flash: $(BUILD_DIR)/$(TARGET)
	sudo openocd -f interface/stlink.cfg -c "set CPUTAPID 0x1ba01477" -f target/stm32f1x.cfg -c "program $< verify reset exit"

# Atajos: compilan Y flashean el modo correcto en un solo comando. Usalos en vez de
# "make MODE=demo && make flash" (el segundo make, sin MODE, recompila a deadman).
flash-deadman:
	$(MAKE) MODE=deadman flash
flash-demo:
	$(MAKE) MODE=demo flash
flash-diag:
	$(MAKE) MODE=deadman DIAG=1 flash

openocd:
	openocd -f interface/stlink.cfg -f target/stm32f1x.cfg

gdb: $(BUILD_DIR)/$(TARGET)
	gdb-multiarch $< -ex "target remote localhost:3333"

size: $(BUILD_DIR)/$(TARGET)
	arm-none-eabi-size $<

clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "Comandos disponibles:"
	@echo "  make                -> Compila la variante deadman"
	@echo "  make flash          -> Compila (deadman) y flashea via OpenOCD"
	@echo "  make flash-deadman  -> Compila Y flashea la variante (Etapa 3/4)"
	@echo "  make flash-demo     -> Compila Y flashea el modo demo (Etapa 2)"
	@echo "  make flash-diag     -> Compila Y flashea con diagnostico de entradas"
	@echo "  make gdb            -> Conecta GDB al target"
	@echo "  make openocd        -> Lanza el servidor OpenOCD"
	@echo "  make size           -> Muestra el uso de memoria"
	@echo "  make clean          -> Limpia la carpeta bin/"
	@echo "  make help           -> Muestra esta ayuda"
