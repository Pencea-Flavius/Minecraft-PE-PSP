BINOUT = ./bin/
PATHSRC = ./src/

TARGET_NAME = kcall
TARGET = $(BINOUT)$(TARGET_NAME)

CC = psp-gcc

PSPSDK = $(shell psp-config --pspsdk-path)
CFLAGS = -fsingle-precision-constant -Ofast -G0 -Wall -I$(PSPSDK)/include
LDFLAGS = -Wl,-q,-T$(PSPSDK)/lib/linkfile.prx -nostartfiles -Wl,-zmax-page-size=128

release: all
	psp-build-exports -s $(PATHSRC)exports.exp && \
	mv $(TARGET_NAME).S ../ && \
	mv bin/$(TARGET_NAME).prx ../bin

C_FILES = $(shell ls $(PATHSRC)*.c)
OBJS = $(notdir $(patsubst %.c, %.o, $(C_FILES)))
OBJS := $(sort $(OBJS:%.o=$(BINOUT)%.o))
OBJS += $(BINOUT)exports.o

FINAL_TARGET = $(TARGET).prx

all: $(FINAL_TARGET)

$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

%.prx: %.elf
	psp-prxgen $< $@

$(BINOUT)%.o: $(PATHSRC)%.c
	$(CC) -o $@ -c $< $(CFLAGS)

%.c: %.exp
	psp-build-exports -b $< > $@

clean:
	-rm -f $(FINAL_TARGET) $(TARGET).elf $(OBJS)

rebuild: clean all
