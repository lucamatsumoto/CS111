#NAME: Luca Matsumoto
#ID: 204726167
#EMAIL: lucamatsumoto@gmail.com

.SILENT:

default:
	gcc -g -Wall -Wextra -lmraa lab4c_tcp.c -o lab4c_tcp -lm
	gcc -g -Wall -Wextra -lmraa -lcrypto -lssl lab4c_tls.c -o lab4c_tls -lm

clean:
	rm -rf *.o *.tar.gz lab4c_tcp lab4c_tls

dist: default
	tar -cvzf lab4c-204726167.tar.gz lab4c_tls.c lab4c_tcp.c README Makefile

