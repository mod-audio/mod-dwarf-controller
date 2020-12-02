# project name
PRJNAME = mod-dwarf-controller

# toolchain configuration
TOOLCHAIN_PREFIX = arm-none-eabi-

# cpu configuration
THUMB = -mthumb
MCU = cortex-m3

# build configuration
mod=$(MAKECMDGOALS)
ifeq ($(mod),$(filter $(mod),moddwarf))
CPU = LPC1778
CPU_SERIE = LPC177x_8x
endif

# target configuration
TARGET_ADDR = root@modduo.local

# project directories
DEVICE_INC	= ./nxp-lpc
CMSIS_INC	= ./nxp-lpc/CMSISv2p00_$(CPU_SERIE)/inc
CMSIS_SRC	= ./nxp-lpc/CMSISv2p00_$(CPU_SERIE)/src
CDL_INC 	= ./nxp-lpc/$(CPU_SERIE)Lib/inc
CDL_SRC 	= ./nxp-lpc/$(CPU_SERIE)Lib/src
APP_INC 	= ./app/inc
APP_SRC 	= ./app/src
RTOS_SRC	= ./freertos/src
RTOS_INC	= ./freertos/inc
DRIVERS_INC	= ./drivers/inc
DRIVERS_SRC	= ./drivers/src
PROTOCOL_INC = ./mod-controller-proto
OUT_DIR		= ./out

CDL_LIBS = lpc177x_8x_clkpwr.c
CDL_LIBS += lpc177x_8x_adc.c lpc177x_8x_gpio.c  lpc177x_8x_pinsel.c
CDL_LIBS += lpc177x_8x_systick.c lpc177x_8x_timer.c
CDL_LIBS += lpc177x_8x_uart.c lpc177x_8x_ssp.c
CDL_LIBS += lpc177x_8x_eeprom.c

SRC = $(wildcard $(CMSIS_SRC)/*.c) $(addprefix $(CDL_SRC)/,$(CDL_LIBS)) $(wildcard $(RTOS_SRC)/*.c) \
	  $(wildcard $(DRIVERS_SRC)/*.c) $(wildcard $(APP_SRC)/*.c)

# Object files
OBJ = $(SRC:.c=.o)
ALL_OBJ = `find -name "*.o"`

# include directories
INC = $(DEVICE_INC) $(CMSIS_INC) $(CDL_INC) $(RTOS_INC) $(DRIVERS_INC) $(APP_INC) $(USB_INC) $(PROTOCOL_INC)

# C flags
CFLAGS += -mcpu=$(MCU)
CFLAGS += -Wall -Wextra -Wpointer-arith -Wredundant-decls -Wsizeof-pointer-memaccess
CFLAGS += -Wa,-adhlns=$(addprefix $(OUT_DIR)/, $(notdir $(addsuffix .lst, $(basename $<))))
CFLAGS += -MMD -MP -MF $(OUT_DIR)/dep/$(@F).d
CFLAGS += -I. $(patsubst %,-I%,$(INC))
CFLAGS += -D$(CPU_SERIE)
CFLAGS += -O2


# Linker flags
LDFLAGS = -Wl,-Map=$(OUT_DIR)/$(PRJNAME).map,--cref
LDFLAGS += -specs=rdimon.specs
LDFLAGS += -Wl,--start-group -lgcc -lc -lm -lrdimon -Wl,--end-group
LDFLAGS += -T./link/LPC.ld

# Define programs and commands.
CC      = $(TOOLCHAIN_PREFIX)gcc
OBJCOPY = $(TOOLCHAIN_PREFIX)objcopy
OBJDUMP = $(TOOLCHAIN_PREFIX)objdump
NM      = $(TOOLCHAIN_PREFIX)nm
SIZE    = $(TOOLCHAIN_PREFIX)size

# define the output files
ELF = $(OUT_DIR)/$(PRJNAME).elf
BIN = $(OUT_DIR)/$(PRJNAME).bin
HEX = $(OUT_DIR)/$(PRJNAME).hex
SYM = $(OUT_DIR)/$(PRJNAME).sym
LSS = $(OUT_DIR)/$(PRJNAME).lss

# Colors definitions
GREEN 	= '\e[0;32m'
NOCOLOR	= '\e[0m'

ifeq ($(mod),)
all:
	@echo -e "Usage: to build, use one of the following:"
	@echo -e "\tmake moddwarf"
else
all: prebuild build
endif

moddwarf: all

build: elf lss sym hex bin

# output files
elf: $(OUT_DIR)/$(PRJNAME).elf
lss: $(OUT_DIR)/$(PRJNAME).lss
sym: $(OUT_DIR)/$(PRJNAME).sym
hex: $(OUT_DIR)/$(PRJNAME).hex
bin: $(OUT_DIR)/$(PRJNAME).bin

prebuild:
	@touch .last_built
ifneq ($(shell cat .last_built),$(mod))
	@make clean
endif
	@mkdir -p $(OUT_DIR)
	@mkdir -p $(OUT_DIR)/dep
	@ln -fs ./$(CPU).ld ./link/LPCmem.ld
	@ln -fs ./config-$(mod).h ./app/inc/config.h
	@echo $(mod) > .last_built

# Create final output file in ihex format from ELF output file (.hex).
$(HEX): $(ELF)
	@echo -e ${GREEN}Creating HEX${NOCOLOR}
	@$(OBJCOPY) -O ihex $< $@

# Create final output file in raw binary format from ELF output file (.bin)
$(BIN): $(ELF)
	@echo -e ${GREEN}Creating BIN${NOCOLOR}
	@$(OBJCOPY) -O binary $< $@

# Create extended listing file/disassambly from ELF output file.
# using objdump (testing: option -C)
$(LSS): $(ELF)
	@echo -e ${GREEN}Creating listing file/disassambly${NOCOLOR}
	@$(OBJDUMP) -h -S -C -r $< > $@

# Create a symbols table from ELF output file.
$(SYM): $(ELF)
	@echo -e ${GREEN}Creating symbols table${NOCOLOR}
	@$(NM) -n $< > $@

# Link: create ELF output file from object files.
$(ELF): $(OBJ)
	@echo -e ${GREEN}Linking objects: generating ELF${NOCOLOR}
	@$(CC) $(THUMB) $(CFLAGS) $(OBJ) --output $@ -nostartfiles $(LDFLAGS)

%.o: %.c
	@echo -e ${GREEN}Building $<${NOCOLOR}
	@$(CC) $(THUMB) $(CFLAGS) -c $< -o $@

clean:
	@echo -e ${GREEN}Object files cleaned out $<${NOCOLOR}
	@rm -rf $(ALL_OBJ) $(OUT_DIR)

size:
	@$(SIZE) out/mod-controller.elf

install:
	scp $(OUT_DIR)/$(PRJNAME).bin $(TARGET_ADDR):/tmp && \
	ssh $(TARGET_ADDR) 'hmi-update /tmp/$(PRJNAME).bin && systemctl restart mod-ui'
