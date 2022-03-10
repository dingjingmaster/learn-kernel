CC = arm-linux-gnueabihf-gcc
CUR_DIR = $(shell pwd)
SRC = $(strip $(subst $(CUR_DIR), ., $(wildcard $(CUR_DIR)/*.c)))
TARGET = $(strip $(patsubst %.c, %.run, $(SRC)))

all:${TARGET}

%.run:%.c
	${CC} -o $@ $<

clean:
	rm -f *.run
	rm -f *.o
