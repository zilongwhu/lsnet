all: liblsnet.a tester

headers=include/*.h

src/error.o: src/error.c
	gcc -g -Wall -Iinclude -DLOG_LEVEL=0 -DDEBUG_EPEX -c $^ -o $@
src/exnet.o: src/exnet.c
	gcc -g -Wall -Iinclude -DLOG_LEVEL=0 -DDEBUG_EPEX -c $^ -o $@
src/hash.o: src/hash.c
	gcc -g -Wall -Iinclude -DLOG_LEVEL=0 -DDEBUG_EPEX -c $^ -o $@
src/log.o: src/log.c
	gcc -g -Wall -Iinclude -DLOG_LEVEL=0 -DDEBUG_EPEX -c $^ -o $@
src/mempool.o: src/mempool.c
	gcc -g -Wall -Iinclude -DLOG_LEVEL=0 -DDEBUG_EPEX -c $^ -o $@
src/timer.o: src/timer.c
	gcc -g -Wall -Iinclude -DLOG_LEVEL=0 -DDEBUG_EPEX -c $^ -o $@
src/utils.o: src/utils.c
	gcc -g -Wall -Iinclude -DLOG_LEVEL=0 -DDEBUG_EPEX -c $^ -o $@

liblsnet.a: src/error.o src/exnet.o src/hash.o src/log.o src/mempool.o src/timer.o src/utils.o
	ar crs $@ $^

tester: liblsnet.a test/timer_tester.c
	gcc -g -Wall -Iinclude -DLOG_LEVEL=0 -DDEBUG_EPEX test/timer_tester.c -lpthread -Xlinker "-(" liblsnet.a -Xlinker "-)" -o $@

clean:
	rm -rf src/*.o liblsnet.a tester
