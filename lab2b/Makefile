#NAME: Luca Matsumoto
#ID: 204726167
#EMAIL: lucamatsumoto@gmail.com

.SILENT:

default: 
	gcc -g -Wall -Wextra -pthread -lprofiler lab2_list.c SortedList.c -o lab2_list

tests:
	chmod +x tester.sh
	./tester.sh

graphs: tests
	chmod +x lab2_list.gp
	./lab2_list.gp

profile:
	rm -f profile.gperf profile.out
	LD_PRELOAD=/usr/lib64/libprofiler.so \
	CPUPROFILE=./raw.gperf \
	./lab2_list --iterations=1000 --threads=12 --sync=s
	pprof --text ./lab2_list ./raw.gperf > profile.out
	pprof --list=sorted_list_action ./lab2_list ./raw.gperf >> profile.out
	rm -f ./raw.gperf

clean:
	rm -rf *.o lab2_list  *.tar.gz

dist: default profile graphs
	tar -czvf lab2b-204726167.tar.gz lab2_list.c SortedList* tester.sh *.png README Makefile lab2b_list.csv profile.out lab2_list.gp 
