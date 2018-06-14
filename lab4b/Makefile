#NAME: Luca Matsumoto
#ID: 204726167
#EMAIL: lucamatsumoto@gmail.com

.SILENT:

default:
	gcc -g -Wall -Wextra -lmraa -lm lab4b.c -o lab4b

clean:
	rm -f *.o lab4b *.tar.gz

check: default 
	chmod +x smokeCheck.sh
	./smokeCheck.sh
	
dist:
	tar -czvf lab4b-204726167.tar.gz Makefile README lab4b.c smokeCheck.sh
