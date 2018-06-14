#NAME: Luca Matsumoto, John Goodlad
#EMAIL: lucamatsumoto@gmail.com
#ID: 204726167

.SILENT:

default:
	gcc -g -Wall -Wextra -std=gnu99 lab3a.c -o lab3a

dist:
	# tar -cvzf lab3a-204726167.tar.gz lab3a.c ext2_fs.h Makefile README
	tar -cvzf lab3a-404675483.tar.gz lab3a.c ext2_fs.h Makefile README

clean:
	rm -f lab3a *.tar.gz *_.csv

test: default
	./lab3a trivial.img | sort > 1.txt
	cat < trivial.csv | sort > 2.txt
	diff 1.txt 2.txt
	
