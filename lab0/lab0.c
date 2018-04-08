//Name: Luca Matsumoto
//Student ID: 204726167
//Email: lucamatsumoto@gmail.com


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

void readWriteFiles(){
    char buf;
    ssize_t stat = read(0, &buf, 1);
    while(stat > 0){
        write(1, &buf, 1);
        stat = read(0, &buf, 1);
    }
}
//function to read and write the files

void signal_handler(int sig){
    if(sig == SIGSEGV){
        fprintf(stderr, "Segmentation fault occured\n");
        exit(4);
    }
}

void inputRedirections(char* inputFile){
    int ifd = open(inputFile, O_RDONLY);
    if(ifd >= 0){
        close(0);
        dup(ifd);
        close(ifd);
    }
    else {
        fprintf(stderr, "--input option failed: Could not open file %s. error number = %d, message = %s \n", inputFile, errno, strerror(errno));//change message
        exit(2);
    }
} //referenced from https://web.cs.ucla.edu/classes/spring18/cs111/projects/fd_juggling.html
//input redirection

void outputRedirections(char* outputFile){
    int ofd = creat(outputFile, 0666);
    if(ofd >= 0){
        close(1);
        dup(ofd);
        close(ofd);
    }
    else {
        fprintf(stderr,"--output option failed: unable to create file %s. error number = %d, message = %s \n", outputFile, errno, strerror(errno));//change message
        exit(3);
    }
}//referenced from https://web.cs.ucla.edu/classes/spring18/cs111/projects/fd_juggling.html
//output redirection

int main(int argc, char** argv) {
    //referenced https://www.gnu.org/software/libc/manual/html_node/Getopt-Long-Option-Example.html
    //required option is 1 not required is 0
    int checkSeg = 0;
    char* input = NULL;
    char* output = NULL;
    int opt = 0;
    static struct option options[] = {
        {"input", 1, 0, 'i'},
        {"output", 1, 0, 'o'},
        {"segfault", 0, 0, 's'}, //segfault is optional
        {"catch", 0, 0, 'c'}, //catch is optional
	{0, 0, 0, 0}
    };
    //colon after required arguments
    while((opt = getopt_long(argc, argv, "i:o:sc", options, NULL))!= -1){
        switch(opt){
            case 'i':
                input = optarg;
                break;
            case 'o':
                output = optarg;
                break;
            case 's':
                checkSeg = -1;
                break;
            case 'c':
                signal(SIGSEGV, signal_handler);
                break;
            default:
                printf("Incorrect argument: correct usage is ./lab0 --input filename --output filename [--segfault --catch] \n");
                exit(1);
        }
    }
    if(input){
        inputRedirections(input);
    }
    if(output){
        outputRedirections(output);
    }
    if(checkSeg == -1){
        char* setPointer = NULL;
        *setPointer = 's';
       // alternative could be to do raise(SIGSEGV)
       //cause segmentation fault
    }
    readWriteFiles();
    exit(0);
}

