#Name: Luca Matsumoto
#ID: 204726167
#Email: lucamatsumoto@gmail.com

.SILENT:

default: server client

server:
	gcc -g -Wall -Wextra lab1b-server.c -lz -o lab1b-server
client:
	gcc -g -Wall -Wextra lab1b-client.c -lz -o lab1b-client

clean:
	rm -f *.o lab1b-client lab1b-server *.tar.gz

dist:
	tar -czf lab1b-204726167.tar.gz lab1b-client.c lab1b-server.c Makefile README

