UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	CC=clang
	CFLAGS=-dynamiclib -fPIC -O2 -Wall
	LDFLAGS=
	OUT=../preload.dylib
else
	CC=gcc
	CFLAGS=-shared -fPIC -O2 -Wall
	LDFLAGS=-ldl
	OUT=../preload.so
endif

all: $(OUT)

$(OUT): preload.c
	$(CC) $(CFLAGS) -o $(OUT) preload.c $(LDFLAGS)

clean:
	rm -f $(OUT) ../preload.dylib ../preload.so
