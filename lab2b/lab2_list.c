//NAME: Luca Matsumoto
//ID: 204726167
//EMAIL: lucamatsumoto@gmail.com

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <time.h>
#include <string.h>
#include "SortedList.h"
#include <pthread.h>
#include <signal.h>
#include <errno.h>

int iterations = 1;
int numThreads = 1;
int numList = 1;
int mutexFlag = 0;
int spinLockFlag = 0;
pthread_mutex_t* mutexes;
SortedList_t* lists;
SortedListElement_t* elements;
int* spinLocks;
int numElements = 0;
char test[32] = "list-";
int opt_yield = 0;
char* yieldOpts = NULL;
long long runTime = 0;
int* nums;

#define ONE_BILLION 1000000000L;

void print_csv_line(char* test, int threadNum, int iterations, int numList, int numOperation, long long runTime, long long avgTime, long long avgMutexTime){
    fprintf(stdout, "%s,%d,%d,%d,%d,%lld,%lld,%lld\n", test, threadNum, iterations, numList, numOperation, runTime, avgTime, avgMutexTime);
}

void getTestName(){
    if(yieldOpts == NULL){
        yieldOpts = "none";
    }
    strcat(test, yieldOpts);
    if(mutexFlag){
        strcat(test, "-m");
    }
    else if(spinLockFlag){
        strcat(test, "-s");
    }
    else if(!mutexFlag && !spinLockFlag){
        strcat(test, "-none");
    }
}

void print_errors(char* error){
    if(strcmp(error, "clock_gettime") == 0){
        fprintf(stderr, "Error initializing clock time\n");
        exit(1);
    }
    if(strcmp(error, "thread_create") == 0){
        fprintf(stderr, "Error creating threads.\n");
        exit(2);
    }
    if(strcmp(error, "thread_join") == 0){
        fprintf(stderr, "Error with pthread_join.\n");
        exit(2);
    }
    if(strcmp(error, "mutex") == 0){
        fprintf(stderr, "Error with pthread_join. \n");
        exit(2);
    }
    if(strcmp(error, "segfault") == 0){
        fprintf(stderr, "Segmentation fault caught! \n");
        exit(2);
    }
    if(strcmp(error, "size") == 0){
        fprintf(stderr, "Sorted List length is not zero. List Corrupted\n");
        exit(2);
    }
    if(strcmp(error, "lookup") == 0){
        fprintf(stderr, "Could not retrieve inserted element due to corrupted list.\n");
        exit(2);
    }
    if(strcmp(error, "length") == 0){
        fprintf(stderr, "Could not retrieve length because list is corrupted.\n");
        exit(2);
    }
    if(strcmp(error, "delete") == 0){
        fprintf(stderr, "Could not delete due to corrupted list. \n");
        exit(2);
    }
}

void signal_handler(int sigNum){
    if(sigNum == SIGSEGV){
        print_errors("segfault");
    }
}//signal handler for SIGSEGV

char* getRandomKey(){
    char* random_key = (char*) malloc(sizeof(char)*2);
    random_key[0] = (char) rand()%26 + 'a';
    random_key[1] = '\0';
    return random_key;
}//https://stackoverflow.com/questions/19724346/generate-random-characters-in-c generating random characters in C

int hashFunction(const char* keys){
    return keys[0]%numList;
}

void setupLocks(){
    if(mutexFlag){
        mutexes = malloc(sizeof(pthread_mutex_t)*numList);
    }
    else if(spinLockFlag){
        spinLocks = malloc(sizeof(int)*numList);
    }
    int i;
    for(i = 0; i < numList; i++){
        if(mutexFlag){
            if(pthread_mutex_init(&mutexes[i], NULL) < 0){
                print_errors("mutex");
            }
        }
        else if(spinLockFlag){
            spinLocks[i] = 0;
        }
        else {
            break;
        }
    }
}


void initializeSubLists(){
    lists = malloc(sizeof(SortedList_t)*numList);
    int i;
    for (i = 0; i < numList; i++){
        lists[i].prev = &lists[i];
        lists[i].next = &lists[i];
        lists[i].key = NULL;
    }
}

void initializeElements(int numElements){
    int i;
    for(i = 0; i < numElements; i++){
        elements[i].key = getRandomKey();
    }
} //initializes elements with random key

void splitElements(){
    nums = malloc(sizeof(int)* numElements);
    int i;
    for(i = 0; i < numElements; i++){
        nums[i] = hashFunction(elements[i].key);
    }
}//uae the hash function to choose which elements go in which list

void initializeLists() {
    initializeSubLists();
    setupLocks();
    initializeElements(numElements);
    splitElements();
}//method to initialize lists, keys, locks

void* sorted_list_action(void* thread_id){
    struct timespec start_time, end_time;
    int i,j;
    int num = *((int*)thread_id);
    for(i = num; i < numElements; i += numThreads){
        if(mutexFlag){
            if(clock_gettime(CLOCK_MONOTONIC, &start_time) < 0){
                print_errors("clock_gettime");
            }
            pthread_mutex_lock(&mutexes[nums[i]]);
            if(clock_gettime(CLOCK_MONOTONIC, &end_time) < 0){
                print_errors("clock_gettime");
            }
            runTime +=  (end_time.tv_sec - start_time.tv_sec) * ONE_BILLION;
            runTime += end_time.tv_nsec;
            runTime -= start_time.tv_nsec; //get lock run time
            SortedList_insert(&lists[nums[i]], &elements[i]);
            pthread_mutex_unlock(&mutexes[nums[i]]);
        }
        else if(spinLockFlag){
            if(clock_gettime(CLOCK_MONOTONIC, &start_time) < 0){
                print_errors("clock_gettime");
            }
            while(__sync_lock_test_and_set(&spinLocks[nums[i]], 1));
            if(clock_gettime(CLOCK_MONOTONIC, &end_time) < 0){
                print_errors("clock_gettime");
            }
            runTime += (end_time.tv_sec - start_time.tv_sec) * ONE_BILLION;
            runTime += end_time.tv_nsec;
            runTime -= start_time.tv_nsec; //get run time
            SortedList_insert(&lists[nums[i]], &elements[i]);
            __sync_lock_release(&spinLocks[nums[i]]);
        }
        else if(!spinLockFlag && !mutexFlag){
            SortedList_insert(&lists[nums[i]], &elements[i]);
        }
    }
    int length = 0;
    if(mutexFlag){
        for(j = 0; j < numList; j++){
            if(clock_gettime(CLOCK_MONOTONIC, &start_time) < 0){
                print_errors("clock_gettime");
            }
            pthread_mutex_lock(&mutexes[j]);
            if(clock_gettime(CLOCK_MONOTONIC, &end_time) < 0){
                print_errors("clock_gettime");
            }
            runTime +=  (end_time.tv_sec - start_time.tv_sec) * ONE_BILLION;
            runTime += end_time.tv_nsec;
            runTime -= start_time.tv_nsec; //get run time
            int subLength = SortedList_length(&lists[j]);
            if (subLength < 0){
                print_errors("length");
            }
            length += subLength;
            pthread_mutex_unlock(&mutexes[j]);
        }
    }
    else if(spinLockFlag){
        for(j = 0; j < numList; j++){
            if(clock_gettime(CLOCK_MONOTONIC, &start_time) < 0){
                print_errors("clock_gettime");
            }
            while(__sync_lock_test_and_set(&spinLocks[j], 1));
            if(clock_gettime(CLOCK_MONOTONIC, &end_time) < 0){
                print_errors("clock_gettime");
            }
            runTime +=  (end_time.tv_sec - start_time.tv_sec) * ONE_BILLION;
            runTime += end_time.tv_nsec;
            runTime -= start_time.tv_nsec; //get run time
            int subLength = SortedList_length(&lists[j]);
            if (subLength < 0){
                print_errors("length");
            }
            length += subLength;
            __sync_lock_release(&spinLocks[j]);
        }
    }
    else if(!spinLockFlag && !mutexFlag){
        for(j = 0; j < numList; j++){
            int subLength = SortedList_length(&lists[j]);
            if (subLength < 0){
                print_errors("length");
            }
            length += subLength;
        }
    }
    SortedListElement_t* insertedElement;
    num = *((int*)thread_id);
    for(i = num; i < numElements; i += numThreads){
        if(mutexFlag){
            if(clock_gettime(CLOCK_MONOTONIC, &start_time) < 0){
                print_errors("clock_gettime");
            }
            pthread_mutex_lock(&mutexes[nums[i]]);
            if(clock_gettime(CLOCK_MONOTONIC, &end_time) < 0){
                print_errors("clock_gettime");
            }
            runTime +=  (end_time.tv_sec - start_time.tv_sec) * ONE_BILLION;
            runTime += end_time.tv_nsec;
            runTime -= start_time.tv_nsec; //get run time
            insertedElement = SortedList_lookup(&lists[nums[i]], elements[i].key);
            if(insertedElement == NULL){
                print_errors("lookup");
            }
            int res = SortedList_delete(insertedElement);
            if(res == 1){
                print_errors("delete");
            }
            pthread_mutex_unlock(&mutexes[nums[i]]);
        }
        else if(spinLockFlag){
            if(clock_gettime(CLOCK_MONOTONIC, &start_time) < 0){
                print_errors("clock_gettime");
            }
            while(__sync_lock_test_and_set(&spinLocks[nums[i]], 1));
            if(clock_gettime(CLOCK_MONOTONIC, &end_time) < 0){
                print_errors("clock_gettime");
            }
            runTime +=  (end_time.tv_sec - start_time.tv_sec) * ONE_BILLION;
            runTime += end_time.tv_nsec;
            runTime -= start_time.tv_nsec; //get run time
            insertedElement = SortedList_lookup(&lists[nums[i]], elements[i].key);
            if(insertedElement == NULL){
                print_errors("lookup");
            }
            int res = SortedList_delete(insertedElement);
            if(res == 1){
                print_errors("delete");
            }
            __sync_lock_release(&spinLocks[nums[i]]);
        }
        else if(!mutexFlag && !spinLockFlag){
            insertedElement = SortedList_lookup(&lists[nums[i]], elements[i].key);
            if(insertedElement == NULL){
                print_errors("lookup");
            }
            int res = SortedList_delete(insertedElement);
            if(res == 1){
                print_errors("delete");
            }
        }
    }
    return NULL;
}

int main(int argc, char** argv){
    int opt = 0;
    char input;
    static struct option options [] = {
        {"iterations", 1, 0, 'i'},
        {"threads", 1, 0, 't'},
        {"yield", 1, 0, 'y'},
        {"sync", 1, 0, 's'},
        {"lists", 1, 0, 'l'},
        {0, 0, 0, 0}
    };
    while((opt=getopt_long(argc, argv, "i:t:ysl:", options, NULL)) != -1){
        switch(opt){
            case 'i':
                iterations = (int)atoi(optarg);
                break;
            case 't':
                numThreads = (int)atoi(optarg);
                break;
            case 'y':
                yieldOpts = (char*) malloc(sizeof(char)*6);
                yieldOpts = strdup(optarg);
                size_t i;
                for(i = 0; i < strlen(optarg); i++){
                    switch(optarg[i]){
                        case 'i':
                            opt_yield |= INSERT_YIELD;
                            break;
                        case 'd':
                            opt_yield |= DELETE_YIELD;
                            break;
                        case 'l':
                            opt_yield |= LOOKUP_YIELD;
                            break;
                        default:
                            fprintf(stderr, "Invalid argument for yield option. \n");
                            exit(1);
                        }
                }
                break;
            case 's':
                input = *optarg;
                switch(input){
                    case 'm':
                        mutexFlag = 1;
                        break;
                    case 's':
                        spinLockFlag = 1;
                        break;
                    default:
                        fprintf(stderr, "Incorrect argument: correct usage is ./lab2_list --iterations iterations --threads numberOfThreads --yield optString [--sync option] \n");
                        exit(1);
                        break;
                }
                break;
            case 'l':
                numList = (int)atoi(optarg);
                break;
            default:
                fprintf(stderr, "Incorrect argument: correct usage is ./lab2_list --iterations iterations --threads numberOfThreads --yield optString [--sync option] \n");
                exit(1);
                break;
        }
        
    }//https://computing.llnl.gov/tutorials/pthreads/ for pthread help
    signal(SIGSEGV, signal_handler);
    numElements = numThreads*iterations;
    elements = (SortedListElement_t*) malloc(sizeof(SortedListElement_t)*numElements);
    srand((unsigned int) time(NULL)); //must use srand before rand
    initializeLists();
    
    int i;
    pthread_t threads[numThreads];
    int thread_id[numThreads];
    struct timespec start, end;
    if (clock_gettime(CLOCK_MONOTONIC, &start) < 0){
        print_errors("clock_gettime");
    }
    
    for(i = 0; i < numThreads; i++){
        thread_id[i] = i;
        int rc = pthread_create(&threads[i], NULL, sorted_list_action, &thread_id[i]);
        if(rc < 0){
            print_errors("thread_create");
        }
    }
    for(i = 0; i < numThreads; i++){
        int rc = pthread_join(threads[i], NULL);
        if(rc < 0){
            print_errors("thread_join");
        }
    }
    
    if(clock_gettime(CLOCK_MONOTONIC, &end) < 0){
        print_errors("clock_gettime");
    }
    
    long long diff = (end.tv_sec - start.tv_sec) * ONE_BILLION;
    diff += end.tv_nsec;
    diff -= start.tv_nsec; //get run time
    
    for(i = 0; i < numList; i++){
        if(SortedList_length(&lists[i]) != 0){
            print_errors("size");
        }
    }

    int numOpts = 3*iterations*numThreads; //get number of operations
    int numLockOpts = (2*iterations + 1)*numThreads;
    getTestName();
    
    print_csv_line(test, numThreads, iterations, numList, numOpts, diff, diff/numOpts, runTime/numLockOpts);
    
    for(i = 0; i < numElements; i++){
        free((void*)elements[i].key);
    }
    free(elements);
    free(lists);
    if(mutexFlag){
        free(mutexes);
        for(i = 0; i < numList; i++){
            pthread_mutex_destroy(&mutexes[i]);
        }
    }
    if(spinLockFlag){
        free(spinLocks);
    }
    free(nums);
    exit(0);
}



