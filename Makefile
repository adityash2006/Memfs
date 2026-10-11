CC = gcc
CFLAGS = -Wall -Wextra -Iinclude
LIBS = -lfuse3
SRCS = src/operations.c src/memfs.c
TARGET = memfs

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) $(LIBS) -o $(TARGET)

clean:
	rm -f $(TARGET)