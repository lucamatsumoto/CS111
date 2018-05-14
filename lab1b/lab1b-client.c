//Name: Luca Matsumoto
//ID: 204726167
//Email: lucamatsumoto@gmail.com

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <getopt.h>
#include <poll.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <netdb.h>
#include "zlib.h"


struct termios original_attributes;
struct termios new_attributes;
int from_terminal[2];
int to_terminal[2];
char buffer[1024];
pid_t childpid;
int socketfd;
struct sockaddr_in server_address;
struct hostent *server;
int logOpt = 0;
int log_fd = 0;
int compressOpt = 0;
z_stream from_server;
z_stream to_server;

void getErrors(char* error, int error_val, int error_number){
    if(strcmp(error, "write") ==0  && error_val < 0){
        fprintf(stderr, "Write failed. Error: %d, Message: %s\n", error_number, strerror(error_number));
        exit(1);
    }
    if(strcmp(error, "read") == 0 && error_val < 0){
        fprintf(stderr, "Error reading from input. Error: %d, Message: %s\n", error_number, strerror(error_number));
        exit(1);
    }
    if(strcmp(error, "log") == 0  && error_val < 0){
        fprintf(stderr, "failed to write to log. Error: %d, Message: %s\n", error_number, strerror(error_number));
        exit(1);
    }
    if(strcmp(error, "poll")  == 0 && error_val < 0){
        fprintf(stderr, "Error in creating poll: Error: %d, Message: %s\n", error_number, strerror(error_number));
        exit(1);
    }
    if(strcmp(error, "dprintf") == 0 && error_val < 0){
        fprintf(stderr, "Error with dprintf, unable to log to logFile: Error: %d, Message: %s\n", error_number, strerror(error_number));
        exit(1);
    }
}

void cleanup_ends(){
    deflateEnd(&to_server);
    inflateEnd(&from_server);
}

void setup_compression(){
    to_server.zalloc = NULL;
    to_server.zfree = NULL;
    to_server.opaque = NULL;
    if(deflateInit(&to_server, Z_DEFAULT_COMPRESSION) < 0){
        fprintf(stderr, "Failed to initialize compression\n");
        exit(1);
    }
    from_server.zalloc = NULL;
    from_server.zfree = NULL;
    from_server.opaque = NULL;
    if(inflateInit(&from_server) < 0){
        fprintf(stderr, "Failed to initialize compression\n");
        exit(1);
    }
    atexit(cleanup_ends);
}

int compression(int bytes, int comp_bufferSize, char* buffer, char* compressionBuf) {
    int compressedBytes; //in case there is not enough space
    to_server.avail_in = (uInt) bytes;
    to_server.next_in = (Bytef *) compressionBuf;
    to_server.avail_out = comp_bufferSize;
    to_server.next_out = (Bytef *) buffer; //pass by pointer
    do {
        int def = deflate(&to_server, Z_SYNC_FLUSH);
        if(def == Z_STREAM_ERROR){
            fprintf(stderr, "Inconsistent stream state: %s", to_server.msg);
            exit(1);
        }
    }while(to_server.avail_in > 0);
    compressedBytes = comp_bufferSize - to_server.avail_out;
    return compressedBytes;
}//make sure to choose the appropriate ZSYNC option

int decompression(int bytes, int decomp_bufferSize, char* buffer, char* decompressionBuf) {
    int decompressedBytes;
    from_server.avail_in = (uInt) bytes;
    from_server.next_in = (Bytef *) decompressionBuf;
    from_server.avail_out = decomp_bufferSize;
    from_server.next_out = (Bytef *) buffer; //pass by pointer
    do {
        int inf = inflate(&from_server, Z_SYNC_FLUSH);
        if(inf == Z_STREAM_ERROR){
            fprintf(stderr, "Inconsistent stream state: %s", from_server.msg);
            exit(1);
        }
    }while(from_server.avail_in > 0);
    decompressedBytes = decomp_bufferSize - from_server.avail_out;
    return decompressedBytes;
}

void read_write(){ //from keyboard to socket
    struct pollfd polls[2];
    polls[0].fd = STDIN_FILENO;
    polls[0].events = POLLIN | POLLHUP | POLLERR;
    polls[1].fd = socketfd;
    polls[1].events = POLLIN | POLLHUP | POLLERR;
    int ret, res;
    int num;
    int i;
    int logWrite;
    char carriage[2];
    carriage[0] = '\r';
    carriage[1] = '\n';
    char compressionBuf[1024];
    if(compressOpt){
        setup_compression();
    }
    for(;;){
        ret = poll(polls, 2, 0);
        getErrors("poll", ret, errno);
        if(polls[0].revents & POLLIN){
            num = read(polls[0].fd, &buffer, 1024);
            getErrors("read", num, errno);
            for(i = 0; i < num; i++){
                char curr = buffer[i];
                switch(curr){
                    case '\r':
                    case '\n':
                        compressionBuf[i] = carriage[1];
                        res = write(STDOUT_FILENO, &carriage[0], sizeof(char));
                        getErrors("write", res, errno);
                        res = write(STDOUT_FILENO, &carriage[1], sizeof(char));
                        getErrors("write", res, errno);
                        if(!compressOpt){
                            res = write(socketfd, &carriage[1], sizeof(char));
                            getErrors("write", res, errno);
                        }
                        break;
                    default:
                        compressionBuf[i] = curr;
                        res = write(STDOUT_FILENO, &curr, sizeof(char));
                        getErrors("write", res, errno);
                        if(!compressOpt){
                            res = write(socketfd, &curr, sizeof(char));
                            getErrors("write", res, errno);
                        }
                        break;
                }
            }
            if(compressOpt){
                char compressionTemp[1024];
                memcpy(compressionTemp, compressionBuf, num); //copy buffer for compression
                num = compression(num, 1024, compressionBuf, compressionTemp);
                res = write(socketfd, compressionBuf, num);
                getErrors("write", res, errno);
            }
            if(logOpt){
                logWrite = dprintf(log_fd, "SENT %d bytes: ", num);
                getErrors("dprintf", logWrite, errno);
                logWrite = write(log_fd, &compressionBuf, num);
                getErrors("log", logWrite, errno);
                dprintf(log_fd, &carriage[1], sizeof(char));
                getErrors("dprintf", logWrite, errno);
            }
        }
        if(polls[0].fd & (POLLHUP | POLLERR)){
            fprintf(stderr, "Server shut down!\n");
            exit(1);
        }
        if(polls[1].revents & POLLIN){
            num = read(polls[1].fd, &buffer, 1024);
            getErrors("num", num, errno);
            if(num == 0){
                exit(1);
            }
            if(logOpt){
                logWrite = dprintf(log_fd, "RECEIVED %d bytes: ", num);
                getErrors("dprintf", logWrite, errno);
                logWrite = write(log_fd, &buffer, num);
                getErrors("log", logWrite, errno);
                logWrite = write(log_fd, &carriage[1], sizeof(char));
                getErrors("log", logWrite, errno);
            }
            if(compressOpt){
                memcpy(compressionBuf, buffer, num);
                num = decompression(num, 1024, buffer, compressionBuf);
            }
            for(i = 0; i < num; i++){
                char curr = buffer[i];
                switch(curr){
                    case '\r':
                    case '\n':
                        res = write(STDOUT_FILENO, &carriage[0], sizeof(char));
                        getErrors("write", res, errno);
                        res = write(STDOUT_FILENO, &carriage[1], sizeof(char));
                        getErrors("write", res, errno);
                        break;
                    default:
                        res = write(STDOUT_FILENO, &curr, sizeof(char));
                        getErrors("write", res, errno);
                        break;
                }
            }
        }
        if(polls[1].revents & (POLLHUP | POLLERR)){
            fprintf(stderr, "Server shut down!\n");
            exit(1);
        }
    }
}


void save_terminal_attributes(){
    int result = tcgetattr(STDIN_FILENO, &original_attributes);
    if(result < 0){
        fprintf(stderr, "Error in getting attributes. Error: %d, Message: %s\n", errno, strerror(errno));
        exit(1);
    }
}
//help with termios from https://support.sas.com/documentation/onlinedoc/sasc/doc/lr2/tgetatr.htm

void reset(){
    if(tcsetattr(STDIN_FILENO, TCSANOW, &original_attributes) < 0) {
        fprintf(stderr, "Could not set the attributes\n");
        exit(1);
    }
}
//remember to map <cr> or <lf> into <cr><lf> in read and write function
void set_input_mode(){
    save_terminal_attributes();
    atexit(reset);
    tcgetattr(STDIN_FILENO, &new_attributes);
    new_attributes.c_iflag = ISTRIP;
    new_attributes.c_oflag = 0;
    new_attributes.c_lflag = 0;
    int res = tcsetattr(STDIN_FILENO, TCSANOW, &new_attributes);
    if(res < 0){
        fprintf(stderr, "Error with setting attributes. Error: %d, Message: %s\n", errno, strerror(errno));
        exit(1);
    }
}

void createSocket_and_connect(int portNum){
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if(socketfd < 0){
        fprintf(stderr, "Socket creation failed\n");
        exit(1);
    }
    server = gethostbyname("localhost");
    if(server == NULL){
        fprintf(stderr, "Error! No such host!\n");
        exit(0);
    }
    memset((char*) &server_address, 0, sizeof(server_address)); //instead of bzero use memset
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(portNum);
    memcpy((char*) &server_address.sin_addr.s_addr, (char*)server->h_addr, server->h_length);
    if(connect(socketfd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0){
        fprintf(stderr, "Error in establishing connection\n");
        exit(1);
    }
}//help with socket from https://www.geeksforgeeks.org/socket-programming-cc/
//and http://www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/client.c

int main(int argc, char** argv){
    if(isatty(STDIN_FILENO) == 0){ //check if STDIN is a terminal
        fprintf(stderr, "STDIN is not an open file descriptor referring to a terminal. Error: %d, Message: %s\n", errno, strerror(errno));
        exit(1);
    }
    int portNum = 0;
    int portOpt = 0;
    int option;
    
    static struct option options [] = {
        {"port", 1, 0, 'p'},
        {"log", 1, 0, 'l'},
        {"compress", 0, 0, 'c'},
        {0, 0, 0, 0}
    };
    while((option = getopt_long(argc, argv, "p:lc", options, NULL)) != -1){
        switch(option){
            case 'p':
                portOpt = 1;
                portNum = atoi(optarg);
                break;
            case 'l':
                logOpt = 1;
                log_fd = creat(optarg, S_IRWXU);//RWX for owner
                if(log_fd < 0){
                    fprintf(stderr, "Unable to create log file. Error: %d, Message: %s\n", errno, strerror(errno));
                    exit(1);
                }
                break;
            case 'c':
                compressOpt = 1;
                break;
            default:
                fprintf(stderr, "Incorrect argument: correct usage is ./lab1b-client --port=portno [--log=filename] [--compress] \n");
                exit(1);
        }
    }
    if(!portOpt){
        fprintf(stderr, "Please specify host in options\n");
    }
    set_input_mode();
    createSocket_and_connect(portNum);
    read_write();
    exit(0);
}
