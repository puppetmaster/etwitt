edje_cc phone.edc
gcc -I/usr/include/elementary-0 -I/usr/include/ethumb-0 -I/usr/include/efreet-1 -I/usr/include/e_dbus-1 -I/usr/include/ecore-1 -I/usr/include/edje-1 -I/usr/include/evas-1 -I/usr/include/eet-1 -I/usr/include/eina-1 -I/usr/include/eina-1/eina -I/usr/include/emotion-0 -I/usr/include/eio-0 -I/usr/include/epdf -I/usr/include/fribidi -I/usr/include/freetype2 -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -I/usr/include/embryo-1 -I/usr/include/eeze-1 -I/usr/include/dbus-1.0 -I/usr/lib/dbus-1.0/include  -lelementary   -o etwitt etwitt.c  
./etwitt 
