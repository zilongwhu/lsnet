liblsnet.so: src/*.c include/*.h
	#gcc -g -Wall -fPIC -shared -Iinclude -DTIMER_CHECK -DLOG_LEVTL=0 -DDEBUG_EPEX src/error.c src/exnet.c src/hash.c src/log.c src/mempool.c src/timer.c src/utils.c -o liblsnet.so -lpthread
	g++ -g -Wall -fPIC -shared -Iinclude -D__CPLUSPLUS -DTIMER_CHECK -DLOG_LEVTL=0 -DDEBUG_EPEX src/error.c src/exnet.c src/hash.c src/log.c src/mempool.c src/timer.c src/utils.c -o liblsnet.so -lpthread

clean:
	rm liblsnet.so
