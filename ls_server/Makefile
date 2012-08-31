liblssrv: src/*.c
	gcc -g -Wall -fPIC -shared -I../core/include -Iinclude -llsnet -L../core -lpthread src/*.c -o liblssrv.so
clean:
	rm liblssrv.so
