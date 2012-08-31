server: src/server.c
	gcc -g -Wall -I../core/include -llsnet -L../core -lpthread src/server.c -o server
server2: src/server2.c
	gcc -g -Wall -I../core/include -I../ls_server/include -llsnet -L../core -llssrv -L../ls_server -lpthread src/server2.c -o server2
clean:
	rm server
	rm server2
