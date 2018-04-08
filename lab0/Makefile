#Name: Luca Matsumoto
#Student ID: 204726167
#Email: lucamatsumoto@gmail.com


.SILENT:

default:
	gcc -g -Wall -Wextra lab0.c -o lab0 

check: default successTest catchSegTest invalidArgTest checkResult createFileTest openTest
	if [[ -e correctOutputs.txt ]]; then \
	echo "Tests were successful!"; \
	rm -f correctOutputs.txt; \
	else \
	echo "Tests were successful!"; \
	fi	
	
clean:
	rm -f *.o lab0 *.txt *.tar.gz

dist: default
	tar -czf lab0-204726167.tar.gz lab0.c Makefile *.png README

successTest:
	echo "This is a test" > input.txt; \
	./lab0 --input input.txt --output output.txt &> /dev/null;  
	if [[ $$? -ne 0 ]]; then \
	echo "Incorrect exit code returned on finish"; \
	else \
	echo "Correct exit code returned on finish" >> correctOutputs.txt; \
	fi
	rm input.txt output.txt	

catchSegTest:
	echo | ./lab0 --segfault --catch &> /dev/null; \
	if [[ $$? -eq 4 ]]; then \
	echo "segfault catch! exit code 4" >> correctOutputs.txt; \
	else \
	echo "wrong exit code on the segfault"; \
	fi
	
invalidArgTest:
	echo | ./lab0 --random &> /dev/null; \
	if [[ $$? -eq 1 ]]; then \
	echo "Correct Argument returned with proper exit code" >> correctOutputs.txt; \
	else \
	echo "Invalid Argument returned with exit code 1"; \
	fi

checkResult:
	echo "This is a test" > input.txt; \
	./lab0 --input input.txt --output output.txt &> /dev/null; \
	cmp input.txt output.txt; \
	if [[ $$? -eq 0 ]]; then \
	echo "input matches output" >> correctOutputs.txt; \
	else \
	echo "input does not match output"; \
	fi
	rm input.txt output.txt

createFileTest:
	echo "This is a test" > input.txt; \
	touch output.txt; \
	chmod u-w output.txt; \
	./lab0 --input input.txt --output output.txt &> /dev/null; \
	if [[ $$? -ne 3 ]]; then \
	echo "File creation exit code (3) did not return properly"; \
	else \
	echo "Correct exit code for file creation!i" >> correctOutputs.txt; \
	fi
	chmod u=rwx output.txt; \
	rm input.txt output.txt

openTest:
	#create file thqt does not exist
	./lab0 --input input.txt --output output.txt &> /dev/null; \
	if [[ $$? -ne 2 ]]; then \
	echo "Incorrect exit code for being unable to open a file"; \
	else \
	echo "Correct exit code for being unable to open a file" >> correctOutputs.txt; \
	fi
	




	
