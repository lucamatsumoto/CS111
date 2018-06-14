#NAME: Luca Matsumoto, John Goodlad
#EMAIL: lucamatsumoto@gmail.com, JohnGoodlad.ucla.edu
#ID: 204726167, 404675483

default: lab3b.py
	rm -f lab3b
	ln -s lab3b.py lab3b
	chmod +x lab3b
	echo "Compilation successful"

clean:
	rm -f lab3b *.tar.gz

dist: default
	tar -cvzf lab3b-204726167.tar.gz lab3b.py Makefile README

test:
	chmod +x test.sh
	./test.sh
