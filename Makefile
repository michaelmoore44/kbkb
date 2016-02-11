#
#       !!!! Do NOT edit this makefile with an editor which replace tabs by spaces !!!!   
#
##############################################################################################
#
# On command line:
#
# make all = Create project
#
# make clean = Clean project files.
#
# To rebuild project do "make clean" and "make all".
#
# Included originally in the yagarto projects. Original Author : Michael Fischer
# Modified to suit our purposes by Hussam Al-Hertani
# Use at your own risk!!!!!
##############################################################################################
# Start of default section
#
CCPREFIX = arm-none-eabi-
CC   = $(CCPREFIX)gcc
AS   = $(CCPREFIX)gcc -x assembler-with-cpp
SIZE = $(CCPREFIX)size
CP   = $(CCPREFIX)objcopy
GDB  = $(CCPREFIX)gdb
HEX  = $(CP) -O ihex
BIN  = $(CP) -O binary -S

OPENOCD = openocd

MCU  = cortex-m4
 
# List all C defines here
DDEFS = 
#Indicate the particular processor used
PROCESSOR = STM32F405xx

#
# Define project name and Ram/Flash mode here
PROJECT        = kbkb
 
# List C source files here

SRC  = ./src/main.c
SRC += ./src/stm32f4xx_it.c
SRC += ./src/system_stm32f4xx.c
SRC += ./src/b2b_comm.c
SRC += ./src/buffer.c
SRC += ./src/keys.c
SRC += ./src/terminal.c
SRC += ./src/usb_device.c
SRC += ./src/usbd_conf.c
SRC += ./src/usbd_desc.c
SRC += ./src/stm32f4xx_hal_msp.c
SRC += ./src/stm32f4xx_hal_driver/stm32f4xx_hal.c
SRC += ./src/stm32f4xx_hal_driver/stm32f4xx_hal_cortex.c
SRC += ./src/stm32f4xx_hal_driver/stm32f4xx_hal_gpio.c
SRC += ./src/stm32f4xx_hal_driver/stm32f4xx_hal_iwdg.c
SRC += ./src/stm32f4xx_hal_driver/stm32f4xx_hal_rcc.c
SRC += ./src/stm32f4xx_hal_driver/stm32f4xx_hal_uart.c
SRC += ./src/stm32f4xx_hal_driver/stm32f4xx_hal_dma.c
SRC += ./src/stm32f4xx_hal_driver/stm32f4xx_hal_pcd.c
SRC += ./src/stm32f4xx_hal_driver/stm32f4xx_hal_pcd_ex.c
SRC += ./src/stm32f4xx_hal_driver/stm32f4xx_ll_usb.c
SRC += ./src/usb_core/usbd_core.c
SRC += ./src/usb_core/usbd_ctlreq.c
SRC += ./src/usb_core/usbd_hid.c
SRC += ./src/usb_core/usbd_ioreq.c

# List assembly startup source file here
STARTUP = ./src/startup_stm32f405xx.s
 
# List all include directories here
INCDIRS  = ./src
INCDIRS += ./src/stm32f4xx_hal_driver
INCDIRS += ./src/cmsis
              
# List the user directory to look for the libraries here
LIBDIRS += 
 
# List all user libraries here
LIBS =
 
# Define optimisation level here
OPT = -Os
 

# Define linker script file here
LINKER_SCRIPT = ./STM32F405VGTx_FLASH.ld

 
INCDIR  = $(patsubst %,-I%, $(INCDIRS))
LIBDIR  = $(patsubst %,-L%, $(LIBDIRS))
LIB     = $(patsubst %,-l%, $(LIBS))


DEFS    = 

OBJS  = $(STARTUP:.s=.o) $(SRC:.c=.o)
MCFLAGS = -mcpu=$(MCU)
 
ASFLAGS = $(MCFLAGS) -g -gdwarf-2 -mthumb  -Wa,-amhls=$(<:.s=.lst) 

CPFLAGS = $(MCFLAGS) $(OPT) -g -gdwarf-2 -mthumb -fomit-frame-pointer 
CPFLAGS += -fdata-sections -ffunction-sections --specs=nano.specs
CPFLAGS += -Wall -Wstrict-prototypes -Wa,-ahlms=$(<:.c=.lst) $(DEFS) -D$(PROCESSOR)

LDFLAGS = $(MCFLAGS) -g -gdwarf-2 -mthumb
LDFLAGS += -T$(LINKER_SCRIPT) -Wl,-Map=$(PROJECT).map,--cref $(LIBDIR) $(LIB)
LDFLAGS += -gc-sections
#LDFLAGS += -nostartfiles
 
#
# makefile rules
#
 
all: $(OBJS) $(PROJECT).elf  $(PROJECT).hex $(PROJECT).bin
	$(SIZE) $(PROJECT).elf
 
%.o: %.c
	$(CC) -c $(CPFLAGS) -I . $(INCDIR) $< -o $@

%.o: %.s
	$(AS) -c $(ASFLAGS) $< -o $@

%.elf: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) $(LIBS) -o $@

%.hex: %.elf
	$(HEX) $< $@
	
%.bin: %.elf
	$(BIN)  $< $@
	

program: $(PROJECT).bin
	@echo "Flash Programming with OpenOCD"
	$(OPENOCD) -f interface\stlink-v2.cfg -f target\stm32f4x.cfg -c "init" -c "reset halt" -c "sleep 100" -c "wait_halt 2" -c "flash write_image erase $(PROJECT).bin 0x08000000" -c "sleep 100" -c "verify_image $(PROJECT).bin 0x08000000" -c "sleep 100" -c "reset run" -c shutdown
	@echo "Flash Programming Finished"
	
erase:
	$(OPENOCD) -f interface\stlink-v2.cfg -f target\stm32f4x.cfg -c "init" -c "reset halt" -c "sleep 100" -c "stm32f3x mass_erase 0" -c "sleep 100" -c shutdown 

oocdserver:
	$(OPENOCD) -f interface\stlink-v2.cfg -f target\stm32f4x.cfg -c "init" -c "halt" -c "reset halt"

oocddbg:
	$(GDB) --eval-command="target remote localhost:3333" $(PROJECT).elf
	
clean:
	-rm -rf $(OBJS)
	-rm -rf $(PROJECT).elf
	-rm -rf $(PROJECT).map
	-rm -rf $(PROJECT).hex
	-rm -rf $(PROJECT).bin
	-rm -rf $(SRC:.c=.lst)
	-rm -rf $(ASRC:.s=.lst)
	
size:$(PROJECT).elf
	@ echo ".size"
	$(SIZE) -A  $(OUTPUT).elf
	
# *** EOF ***
