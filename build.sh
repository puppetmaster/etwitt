CC="gcc"
CFLAGS="-w -Wall"

edje_cc phone.edc
$CC `pkg-config --cflags --libs elementary` $CFLAGS -o etwitt etwitt.c  
$CC `pkg-config --cflags --libs openssl oauth` $CFLAGS -o ebird ebird.c
#./etwitt 
