CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -Wall -Werror
LDFLAGS ?= -lpthread -lrt
TARGET ?= aesdsocket

all: $(TARGET)
default: $(TARGET)

$(TARGET): aesdsocket.c
	$(CC) $(CFLAGS) -o $(TARGET) aesdsocket.c $(LDFLAGS)

clean:
	rm -f $(TARGET)

