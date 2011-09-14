CC="gcc"
CFLAGS="-w -Wall"

edje_cc ./data/theme/phone.edc
$CC `pkg-config --cflags --libs elementary` $CFLAGS -o etwitt src/bin/etwitt.c  
$CC `pkg-config --cflags --libs oauth` `pkg-config --cflags --libs ecore-con` `pkg-config --cflags --libs openssl` $CFLAGS -o ebird src/bin/ebird.c
#./etwitt 
