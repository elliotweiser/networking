TARGETS=obj/ErrorHandling.o obj/Memory.o obj/Http.o obj/HttpServer.o obj/ServerThread.o obj/HttpRequest.o server_prog

CFLAGS=-Wall -g -O0

all: $(TARGETS)

server_prog: server_prog.cpp obj/HttpServer.o obj/ServerThread.o obj/HttpRequest.o obj/Http.o obj/Memory.o obj/ErrorHandling.o
	g++ $(CFLAGS) -pthread server_prog.cpp obj/HttpServer.o obj/ServerThread.o obj/HttpRequest.o obj/Http.o obj/Memory.o obj/ErrorHandling.o -o server_prog

obj/HttpServer.o: src/HttpServer.cpp
	g++ $(CFLAGS) -c src/HttpServer.cpp -o obj/HttpServer.o

obj/ServerThread.o: src/ServerThread.cpp
	g++ $(CFLAGS) -pthread -c src/ServerThread.cpp -o obj/ServerThread.o

obj/HttpRequest.o: src/HttpRequest.cpp
	g++ $(CFLAGS) -c src/HttpRequest.cpp -o obj/HttpRequest.o

obj/Http.o: src/Http.cpp
	g++ $(CFLAGS) -c src/Http.cpp -o obj/Http.o

obj/Memory.o: src/Memory.cpp
	g++ $(CFLAGS) -c src/Memory.cpp -o obj/Memory.o

obj/ErrorHandling.o: src/ErrorHandling.cpp
	g++ $(CFLAGS) -c src/ErrorHandling.cpp -o obj/ErrorHandling.o

clean:
	rm -f obj/* server_prog

