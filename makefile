#Порт для заливки прошивки
OUT_PORT = COM4
MCU = atmega328p

#Флаг оптимизации
OPT_FLAG = O2

#Директории входных и выходных файлов
INC_DIR = source
SRC_DIR = source
OUT_DIR = build

#Задаем из каких файлов собирать проект, можно указать несколько файлов
SRC  = module_uart.c
SRC += module_tenzo.c
SRC += module_drive.c
SRC += module_operations.c
SRC += main.c

#Флаги компилятора, при помощи F_CPU определяем частоту на которой будет работать контроллер
CFLAGS   = -std=gnu99 -mmcu=$(MCU) -g -$(OPT_FLAG) -mcall-prologues -DF_CPU=16000000 -I$(INC_DIR)
LDFLAGS  = -std=gnu99 -mmcu=$(MCU) -g -$(OPT_FLAG) -mcall-prologues -I$(INC_DIR)

OBJS     = $(SRC:.c=.o)
OBJS_OUT = $(addprefix $(OUT_DIR)/, $(OBJS))

OBJS_Z     = $(SRC:.c=_z.o)
OBJS_Z_OUT = $(addprefix $(OUT_DIR)/, $(OBJS_Z))


#-------------------------------------------#
#-------------------------------------------#
#-------------------------------------------#

all: makedir build build_z

build: compile clear

build_z: compile_z clear_z


compile: $(OBJS)
	avr-gcc $(LDFLAGS) -o $(OUT_DIR)/module.elf  $(OBJS_OUT)
	avr-objcopy -O ihex  $(OUT_DIR)/module.elf $(OUT_DIR)/module.hex

%.o:  $(SRC_DIR)/%.c
	@avr-gcc $(CFLAGS) -c $< -o $(OUT_DIR)/$@


compile_z: $(OBJS_Z)
	avr-gcc $(LDFLAGS) -o $(OUT_DIR)/z_drive.elf  $(OBJS_Z_OUT)
	avr-objcopy -O ihex  $(OUT_DIR)/z_drive.elf $(OUT_DIR)/z_drive.hex

%_z.o:  $(SRC_DIR)/%.c
	@avr-gcc $(CFLAGS) -DZ_DRIVE -c $< -o $(OUT_DIR)/$@


makedir:
	@mkdir $(OUT_DIR) 2>NUL || true

clear clear_z:
	@del /q $(OUT_DIR)\*.o 2>NUL || true
	@del /q $(OUT_DIR)\*.elf 2>NUL || true
