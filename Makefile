clean:
	rm *.o

packs.o:
	ld -r -b binary data -o packs.o

packs: packs.o

resource_test: packs.o tests/resource_test.c source/resource.c source/lzrw3.c
	gcc -g -I headers/ tests/resource_test.c source/resource.c source/lzrw3.c packs.o -o resource_test -fsanitize=address -fsanitize=leak

crypt_test: packs.o tests/crypt_test.c source/resource.c source/lzrw3.c
	gcc -g -I headers/ tests/crypt_test.c source/resource.c source/lzrw3.c packs.o -o crypt_test -fsanitize=address -fsanitize=leak -lm
