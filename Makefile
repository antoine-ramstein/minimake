CFLAGS  = -std=c99 -pedantic -Wall -Wextra -Wvla -Werror
SRC = src/main.c src/parse.c src/helpers.c
OBJ = src/main.o src/parse.o src/helpers.o
TARGET = minimake

all: $(TARGET)

$(TARGET): $(OBJ)
	gcc -o $(TARGET) $(OBJ)

src/main.o: src/main.c
	gcc $(CFLAGS) -c src/main.c -o src/main.o

src/parse.o: src/parse.c
	gcc $(CFLAGS) -c src/parse.c -o src/parse.o

src/helpers.o: src/helpers.c
	gcc $(CFLAGS) -c src/helpers.c -o src/helpers.o

check:
	./tests/tests.sh

clean:
	rm -rf $(OBJ) $(TARGET)
