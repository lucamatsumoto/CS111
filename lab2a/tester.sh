#!/bin/bash

#Name: Luca Matsumoto
#ID: 204726167
#Email: lucamatsumoto@gmail.com

rm -f lab2_add.csv
rm -f lab2_list.csv
touch lab2_add.csv
touch lab2_list.csv

#add-none test
for i in 1, 2, 4, 8, 12
do
    for j in 10, 20, 40, 80, 100, 1000, 10000, 100000
    do
        ./lab2_add --iterations=$j --threads=$i >> lab2_add.csv
    done
done

#add-yield-none test
for i in 1, 2, 4, 8, 12
do
        for j in 10, 20, 40, 80, 100, 1000, 10000, 100000
        do
                ./lab2_add --iterations=$j --threads=$i --yield >> lab2_add.csv
        done
done

#add-m test
for i in 1, 2, 4, 8, 12
do
        for j in 10, 20, 40, 80, 100, 1000, 10000, 100000
        do
                ./lab2_add --iterations=$j --threads=$i --sync=m >> lab2_add.csv
        done
done

#add-s test
for i in 1, 2, 4, 8, 12
do
        for j in 10, 20, 40, 80, 100, 1000, 10000, 100000
        do
                ./lab2_add --iterations=$j --threads=$i --sync=s >> lab2_add.csv
        done
done

#add-c test
for i in 1, 2, 4, 8, 12
do
        for j in 10, 20, 40, 80, 100, 1000, 10000, 100000
        do
                ./lab2_add --iterations=$j --threads=$i --sync=c >> lab2_add.csv
        done
done

#add-yield-m test
for i in 1, 2, 4, 8, 12
do
        for j in 10, 20, 40, 80, 100, 1000, 10000, 100000
        do
                ./lab2_add --iterations=$j --threads=$i --yield --sync=m >> lab2_add.csv
        done
done

#add-yield-s test
for i in 1, 2, 4, 8, 12
do
        for j in 10, 20, 40, 80, 100, 1000, 10000, 100000
        do
                ./lab2_add --iterations=$j --threads=$i --yield --sync=s >> lab2_add.csv
        done
done

#add-yield-c test
for i in 1, 2, 4, 8, 12
do
        for j in 10, 20, 40, 80, 100, 1000, 10000, 100000
        do
                ./lab2_add --iterations=$j --threads=$i --yield --sync=c >> lab2_add.csv
        done
done

#single thread test
for j in 10, 100, 1000, 10000, 20000
do
    ./lab2_list --iterations=$j --threads=1 >> lab2_list.csv
done

#numerous threads for lab2_list
for i in 2, 4, 8, 12
do
    for j in 1, 10, 100, 1000
    do
        ./lab2_list --iterations=$j --threads=$i >> lab2_list.csv
    done
done

#yield with i option
for i in 2, 4, 8, 12
do
    for j in 1, 2, 4, 8, 16, 32
    do
        ./lab2_list --iterations=$j --threads=$i --yield=i >> lab2_list.csv
    done
done

#yield with d option
for i in 2, 4, 8, 12
do
        for j in 1, 2, 4, 8, 16, 32
        do
                ./lab2_list --iterations=$j --threads=$i --yield=d >> lab2_list.csv
        done
done

#yield with il option
for i in 2, 4, 8, 12
do
        for j in 1, 2, 4, 8, 16, 32
        do
                ./lab2_list --iterations=$j --threads=$i --yield=il >> lab2_list.csv
        done
done

#yield with dl option
for i in 2, 4, 8, 12
do
        for j in 1, 2, 4, 8, 16, 32
        do
                ./lab2_list --iterations=$j --threads=$i --yield=dl >> lab2_list.csv
        done
done

#yield with i and sync=m

for i in 2, 4, 8, 12
do
        for j in 1, 2, 4, 8, 16, 32
        do
                ./lab2_list --iterations=$j --threads=$i --yield=i --sync=m >> lab2_list.csv
        done
done

#yield with d and sync=m
for i in 2, 4, 8, 12
do
        for j in 1, 2, 4, 8, 16, 32
        do
                ./lab2_list --iterations=$j --threads=$i --yield=d --sync=m >> lab2_list.csv
        done
done

#yield with il and sync=m
for i in 2, 4, 8, 12
do
        for j in 1, 2, 4, 8, 16, 32
        do
                ./lab2_list --iterations=$j --threads=$i --yield=il --sync=m >> lab2_list.csv
        done
done

#yield with dl and sync=m
for i in 2, 4, 8, 12
do
        for j in 1, 2, 4, 8, 16, 32
        do
                ./lab2_list --iterations=$j --threads=$i --yield=dl --sync=m  >> lab2_list.csv
        done
done

#yield with i and sync=s

for i in 2, 4, 8, 12
do
        for j in 1, 2, 4, 8, 16, 32
        do
                ./lab2_list --iterations=$j --threads=$i --yield=i --sync=s >> lab2_list.csv
        done
done

#yield with d and sync=s
for i in 2, 4, 8, 12
do
        for j in 1, 2, 4, 8, 16, 32
        do
                ./lab2_list --iterations=$j --threads=$i --yield=d --sync=s >> lab2_list.csv
        done
done

#yield with il and sync=s
for i in 2, 4, 8, 12
do
        for j in 1, 2, 4, 8, 16, 32
        do
                ./lab2_list --iterations=$j --threads=$i --yield=il --sync=s >> lab2_list.csv
        done
done

#yield with dl and sync=s
for i in 2, 4, 8, 12
do
        for j in 1, 2, 4, 8, 16, 32
        do
                ./lab2_list --iterations=$j --threads=$i --yield=dl --sync=s  >> lab2_list.csv
        done
done

#test for appropriate number of threads to overcome startup costs

for i in 1, 2, 4, 8, 12, 16, 24
do
    ./lab2_list --iterations=1000 --threads=$i >> lab2_list.csv
done

for i in 1, 2, 4, 8, 12, 16, 24
do
        ./lab2_list --iterations=1000 --threads=$i --sync=m >> lab2_list.csv
done

for i in 1, 2, 4, 8, 12, 16, 24
do
        ./lab2_list --iterations=1000 --threads=$i --sync=s >> lab2_list.csv
done

