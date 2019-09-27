TARGET=tftps
SRC=$(wildcard src/*.c)
OBJ=$(patsubst %.c,%.o,$(SRC))
INC=-Iinclude

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $(OBJ)

%.o: %.c
	$(CC) $(INC) -c -o $@ $<

clean:
	$(RM) $(TARGET) $(OBJ)

.PHONY: clean all
