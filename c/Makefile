clean:
	rm *.o resource_test crypt_test

packs.o:
	ld -r -b binary data -o packs.o

packs: packs.o

resource_test: packs.o tests/resource_test.c source/resource.c source/lzrw3-a.c
	gcc -g -I headers/ tests/resource_test.c source/resource.c source/lzrw3-a.c packs.o -o resource_test -fsanitize=address -lm

crypt_test: packs.o tests/crypt_test.c source/resource.c source/lzrw3-a.c
	gcc -g -I headers/ tests/crypt_test.c source/resource.c source/lzrw3-a.c packs.o -o crypt_test -fsanitize=address -lm

quickdraw_test: packs.o tests/quickdraw_test.c source/resource.c source/quickdraw.c source/lzrw3-a.c
	gcc -g -I headers/ tests/quickdraw_test.c source/resource.c source/quickdraw.c source/lzrw3-a.c packs.o -o quickdraw_test -fsanitize=address -lm

sprites_test: packs.o tests/sprites_test.c source/resource.c source/lzrw3-a.c
	gcc -g -I headers/ tests/sprites_test.c source/resource.c source/lzrw3-a.c packs.o -o sprites_test -fsanitize=address -lm
