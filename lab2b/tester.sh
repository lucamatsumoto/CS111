#!/bin/bash

#Name: Luca Matsumoto
#ID: 204726167
#Email: lucamatsumoto@gmail.com

#test for generating throughput of different numbers of threads

rm -rf lab2b_list.csv
touch lab2b_list.csv

for i in 1, 2, 4, 8, 12, 16, 24
do
	./lab2_list --iterations=1000 --threads=$i --sync=m >> lab2b_list.csv
done

for i in 1, 2, 4, 8, 12, 16, 24
do
        ./lab2_list --iterations=1000 --threads=$i --sync=s >> lab2b_list.csv
done

for i in 1, 4, 8, 12, 16
do
	for j in 1, 2, 4, 8, 16
	do
		./lab2_list --iterations=$j --threads=$i --yield=id --lists=4 >> lab2b_list.csv
	done
done

for i in 1, 4, 8, 12, 16
do
        for j in 10, 20, 40, 80
        do
                ./lab2_list --iterations=$j --threads=$i --yield=id --sync=m --lists=4 >> lab2b_list.csv
        done
done

for i in 1, 4, 8, 12, 16
do
        for j in 10, 20, 40, 80
        do
                ./lab2_list --iterations=$j --threads=$i --yield=id --sync=s --lists=4 >> lab2b_list.csv
        done
done

for i in 1, 2, 4, 8, 12
do
        for j in 4, 8, 16
        do
                ./lab2_list --iterations=1000 --threads=$i --sync=m --lists=$j >> lab2b_list.csv
        done
done

for i in 1, 2, 4, 8, 12
do
        for j in 4, 8, 16
        do
                ./lab2_list --iterations=1000 --threads=$i --sync=s --lists=$j >> lab2b_list.csv
        done
done
