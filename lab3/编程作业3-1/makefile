all: client server

client:
	g++ client.cpp -o client.exe -lws2_32

server:
	g++ server.cpp -o server.exe -lws2_32

clean:
	rm -f client.exe server.exe