CC = clang
CFLAGS = -Wall -g -I ../include/
TARGET = main
SRCS = main.c gpt.c mbr.c
OBJS = $(SRCS:.c=.o)

all:$(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) 

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm -rf $(OBJS) $(TARGET)

