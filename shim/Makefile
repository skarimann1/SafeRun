CC=gcc
CFLAGS=-shared -fPIC -O2 -Wall
LDFLAGS=-ldl
OUT=../preload.so

all: $(OUT)

$(OUT): preload.c
	$(CC) $(CFLAGS) -o $(OUT) preload.c $(LDFLAGS)

clean:
	rm -f $(OUT)
