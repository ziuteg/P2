## Treść zadania

Zaimplementuj sterownik obsługi systemu plików FAT16 z użyciem interfejsu FUSE. Struktury danych opisujące reprezentację dyskową można pożyczyć ze sterownika jądra NetBSD. Twój sterownik musi implementować ​wyłącznie ​odczyt danych z obrazu systemu plików.

Przygotuj swoją ulubioną dystrybucję Linuksa (np. Ubuntu) do pracy poprzez instalację pakietów: `dosfstools`,​ `libfuse-dev`. Pełna dokumentacja FUSE jest umieszczo na na stronie zajęć. Przykłady wykorzystania biblioteki FUSE znajdują się w ​`/usr/share/doc/libfuse-dev/examples` Dobrze jest też poczytać źródła innych sterowników FUSE i zaglądać do plików nagłówkowych biblioteki. Do testowania swojego sterownika należy utworzyć obraz systemu plików w następujący sposób:

	$ dd if=/dev/zero of=fs_image.raw ​ count=$[1024*100] bs=1024 
	$ /sbin/mkfs.vfat -F 16 -n SO2015 -v ​ fs_image.raw 
	$ mkdir ​ fs_root 
	$ sudo mount -t msdos -o loop,rw,showexec,codepage=852,umask=0 ​ fs_image.raw ​ ​ fs_root
	$ python fs-populate.py ​ fs_root/
	$ sudo umount ​ fs_root

Znaczenie poszczególnych poleceń:
1. Utwórz plik wypełniony ze rami o objętości 100MiB.
2. W obrębie pliku `fs_image.raw` utwórz system plików FAT16 z etykietą SO2015.
4. Używając sterownika loopback​ zamontuj obraz systemu plików w katalogu `fs_root`.
6. Odmontuj nasz system plików.

Punktem startowym dla swojego sterownika może być przykładowy plik źródłowy​ `hello_ll.c`. W wektorze funkcji​ `fuse_lowlevel_ops` musisz podać wskaźniki do implementacji operacji: ​ `open`, `read`, `release`, `getattr`, `lookup`, `opendir`, `readdir`, `releasedir`, `statfs`. Zadbaj o zwracanie odpowiednich kodów błędu w razie niepowodzenia operacji.

Ważnym ograniczeniem jest to, że musisz traktować plik obrazu systemu plików jak dyskowe urządzenie blokowe z sektorami o wielkości 512 bajtów – tzn. podstawową jednostką transferu z obrazu jest sektor. Innym słowy – ilość wczytywanych danych i pozycja kursora muszą być zawsze podzielne przez 512.

Prezentacja rozwiązania będzie polegała na zamontowaniu obrazu systemu plików z użyciem sterownika i wykonywaniu poleceń ​`stat`, `ls`, `find`, `cat`. Upewnij się, że system plików przechowuje setki plików o różnej wielkości, a struktura katalogów jest nietrywialna – tj. co najmniej 3 poziomy głębokości.