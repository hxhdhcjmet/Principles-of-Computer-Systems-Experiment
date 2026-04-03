CC = gcc
CFLAGS = -O3 -mavx2 -mfma -pthread
TARGET = matmult_v2
SRC = matmult_v2.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)