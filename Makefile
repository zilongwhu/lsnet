all: liblsnet.a tester

headers=include/*.h

error.o: src/error.c
	gcc -c -g -Wall -Iinclude -DLOG_LEVEL=0 -DDEBUG_EPEX $^
exnet.o: src/exnet.c
	gcc -c -g -Wall -Iinclude -DLOG_LEVEL=0 -DDEBUG_EPEX $^
hash.o: src/hash.c
	gcc -c -g -Wall -Iinclude -DLOG_LEVEL=0 -DDEBUG_EPEX $^
log.o: src/log.c
	gcc -c -g -Wall -Iinclude -DLOG_LEVEL=0 -DDEBUG_EPEX $^
mempool.o: src/mempool.c
	gcc -c -g -Wall -Iinclude -DLOG_LEVEL=0 -DDEBUG_EPEX $^
timer.o: src/timer.c
	gcc -c -g -Wall -Iinclude -DLOG_LEVEL=0 -DDEBUG_EPEX $^
utils.o: src/utils.c
	gcc -c -g -Wall -Iinclude -DLOG_LEVEL=0 -DDEBUG_EPEX $^

liblsnet.a: error.o exnet.o hash.o log.o mempool.o timer.o utils.o
	ar crs $@ $^

tester: liblsnet.a test/timer_tester.c
	gcc -g -Wall -Iinclude -DLOG_LEVEL=0 -DDEBUG_EPEX test/timer_tester.c -lpthread -Xlinker "-(" liblsnet.a -Xlinker "-)" -o $@

clean:
	rm -rf *.o liblsnet.a tester
