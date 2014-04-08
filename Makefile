all: liblsnet.a

headers=include/*.h

error.o: src/error.c $(headers)
	gcc -c -g -Wall -Iinclude -DLOG_LEVEL=0 -DDEBUG_EPEX $^
exnet.o: src/exnet.c $(headers)
	gcc -c -g -Wall -Iinclude -DLOG_LEVEL=0 -DDEBUG_EPEX $^
hash.o: src/hash.c $(headers)
	gcc -c -g -Wall -Iinclude -DLOG_LEVEL=0 -DDEBUG_EPEX $^
log.o: src/log.c $(headers)
	gcc -c -g -Wall -Iinclude -DLOG_LEVEL=0 -DDEBUG_EPEX $^
mempool.o: src/mempool.c $(headers)
	gcc -c -g -Wall -Iinclude -DLOG_LEVEL=0 -DDEBUG_EPEX $^
timer.o: src/timer.c $(headers)
	gcc -c -g -Wall -Iinclude -DLOG_LEVEL=0 -DDEBUG_EPEX $^
utils.o: src/utils.c $(headers)
	gcc -c -g -Wall -Iinclude -DLOG_LEVEL=0 -DDEBUG_EPEX $^

liblsnet.a: error.o exnet.o hash.o log.o mempool.o timer.o utils.o
	ar crs $@ $^

clean:
	rm -rf *.o liblsnet.a
