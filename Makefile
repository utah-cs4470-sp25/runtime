.PHONY: all clean

CFILES=$(wildcard *.c)
OFILES=$(patsubst %.c,%.o,$(CFILES))

all: runtime.a

%.o: %.c
	$(CC) -Og -Werror -Wall -Wpedantic -c -o $@ -- $^

runtime.a: $(OFILES)
	ar rcs $@ $^

clean:
	rm $(OFILES) runtime.a
