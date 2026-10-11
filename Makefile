include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk


# $(shell) may not exist on your computer
ELFNAME:= $(shell basename $(PWD))

ELF := $(ELFNAME).elf

CFLAGS := -Wall -Werror -g

PAYLOAD_OBJS := $(wildcard payload_files/*.o)

all: $(ELF)

LDADD := -lSceNotification

SRCS := main.c util.c self.c self-prospero.c io.c sha256.c

$(ELF): $(SRCS) $(PAYLOAD_OBJS)
	$(CC) $(CFLAGS) -o $@ $(LDADD) $(SRCS) $(PAYLOAD_OBJS)
	

test:
	nc -q 0 192.168.0.134 9021 < $(ELF)

clean:
	rm -f $(ELF)
