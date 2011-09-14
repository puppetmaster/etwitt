CC     = gcc
CFLAGS = -Wall -Wextra -g -ggdb -O0

all: phone.edj ebird etwitt

etwitt: src/bin/etwitt.c
	${CC} ${CFLAGS} `pkg-config elementary --cflags --libs` $< -o $@

ebird: src/bin/ebird.c
	${CC} ${CFLAGS} `pkg-config --cflags --libs oauth` `pkg-config --cflags --libs ecore-con` `pkg-config --cflags --libs openssl` $< -o $@

phone.edj: data/theme/phone.edc
	edje_cc $<

