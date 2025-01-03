.PHONY: all clean

CFILES=$(wildcard *.c)
OFILES=$(patsubst %.c,%.o,$(CFILES))

LIBPNG=libpng-1.6.43

CC=clang

all: runtime.a

$(LIBPNG)/configure:
	@echo "Downloading LibPNG\n"
	curl -L https://download.sourceforge.net/libpng/$(LIBPNG).tar.xz | tar -xJ

$(LIBPNG)/target/include/png.h: $(LIBPNG)/configure
	@echo "\nCompiling LibPNG\n"
	(cd libpng-1.6.43 && CC=$(CC) LDFLAGS="" CPPFLAGS="" ./configure --prefix $(PWD)/$(LIBPNG)/target)
	$(MAKE) -C $(LIBPNG) install

%.o: %.c $(LIBPNG)/target/include/png.h
	@echo "\nCompiling RunTime\n"
	$(CC) -I$(LIBPNG)/target/include -Og -Wall -Wpedantic -c -o $@ $<

runtime.a: $(OFILES) $(LIBPNG)/target/lib/libpng16.a
	@echo "\nLinking RunTime\n"
	cp $(LIBPNG)/target/lib/libpng16.a $@
	ar rcs $@ $(OFILES)

clean:
	rm -f $(OFILES) runtime.a
ifneq (,$(wildcard $(LIBPNG)/Makefile))
	$(MAKE) -C $(LIBPNG) clean
endif

fullclean:
	rm -rf $(LIBPNG)
	@$(MAKE) clean
