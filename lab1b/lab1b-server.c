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
#include "zlib.h"

struct termios original_attributes;
struct termios new_attributes;
int from_terminal[2];
int to_terminal[2];
char buffer[1024];
pid_t childpid;
int compressOpt = 0;
struct sockaddr_in server_address, client_address;
int socketfd, new_socket_fd;
z_stream from_client;
z_stream to_client;

void getErrors(char* error, int error_val, int error_number){
    if(strcmp(error, "write") == 0 && error_val < 0){
        fprintf(stderr, "Write failed. Error: %d, Message: %s\n", error_number, strerror(error_number));
        exit(1);
    }
    if(strcmp(error, "read") == 0 && error_val < 0){
        fprintf(stderr, "Error reading from input. Error: %d, Message: %s\n", error_number, strerror(error_number));
        exit(1);
    }
    if(strcmp(error, "log") == 0 && error_val < 0){
        fprintf(stderr, "failed to write to log. Error: %d, Message: %s\n", error_number, strerror(error_number));
        exit(1);
    }
    if(strcmp(error, "poll") == 0 && error_val < 0){
        fprintf(stderr, "Error in creating poll: Error: %d, Message: %s\n", error_number, strerror(error_number));
        exit(1);
    }
    if(strcmp(error, "dprintf") == 0 && error_val < 0){
        fprintf(stderr, "Error with dprintf, unable to log to logFile: Error: %d, Message: %s\n", error_number, strerror(error_number));
        exit(1);
    }
    if(strcmp(error, "write_special") == 0 && error_val < 0){
        fprintf(stderr, "Error with write_special: Error: %d, Message: %s\n", error_number, strerror(error_number));
        exit(1);
    }
}

void print_exit_signal(){
    //shutdown(new_socket_fd, SHUT_RDWR);
    int status;
    int val = waitpid(childpid, &status, 0);
    if(val < 0){
        fprintf(stderr, "Error in waitpid: Error: %d, Message: %s\n", errno, strerror(errno));
        exit(1);
    }
    fprintf(stderr, "SHELL EXIT SIGNAL=%d, STATUS=%d\r\n", WTERMSIG(status), WEXITSTATUS(status));
    close(socketfd);
    close(new_socket_fd);
}

void cleanup_ends(){
    deflateEnd(&to_client);
    inflateEnd(&from_client);
}

void setup_compression(){
    to_client.zalloc = NULL;
    to_client.zfree = NULL;
    to_client.opaque = NULL;
    if(deflateInit(&to_client, Z_DEFAULT_COMPRESSION) < 0){
        fprintf(stderr, "Failed to initialize compression\n. Message: %s", to_client.msg);
        exit(1);
    }
    from_client.zalloc = NULL;
    from_client.zfree = NULL;
    from_client.opaque = NULL;
    if(inflateInit(&from_client) < 0){
        fprintf(stderr, "Failed to initialize compression. Message: %s\n", from_client.msg);
        //Z_MEM_ERROR when not enough memory, Z_STREAM_ERROR when level is not compatible
        //Z_VERSION_ERROR when library is incompatible
        exit(1);
    }
    atexit(cleanup_ends);
}//referenced from zlib tutorial https://www.zlib.net/zpipe.c

int compression(int bytes, int bufferSize, char* buffer, char* compressionBuf) {
    int compressedBytes;
    to_client.avail_in = (uInt) bytes;
    to_client.next_in = (Bytef *) compressionBuf;
    to_client.avail_out = bufferSize;
    to_client.next_out = (Bytef *) buffer;//pass by pointer
    do {
        int def = deflate(&to_client, Z_SYNC_FLUSH);
        if(def == Z_STREAM_ERROR){
            fprintf(stderr, "Inconsistent stream state: %s", to_client.msg);
            exit(1);
        }
    }while(to_client.avail_in > 0);
    compressedBytes = bufferSize - to_client.avail_out;
    return compressedBytes;
}//make sure to choose the appropriate ZSYNC option

int decompression(int bytes, int bufferSize, char* buffer, char* decompresssionBuf) {
    int decompressedBytes;
    from_client.avail_in = (uInt) bytes;
    from_client.next_in = (Bytef *) decompresssionBuf;
    from_client.avail_out = bufferSize;
    from_client.next_out = (Bytef *) buffer; //pass by pointer
    do {
        int inf = inflate(&from_client, Z_SYNC_FLUSH);
        if(inf == Z_STREAM_ERROR){
            fprintf(stderr, "Inconsistent stream state: %s", from_client.msg);
            exit(1);
        }
    }while(from_client.avail_in > 0);
    decompressedBytes = bufferSize - from_client.avail_out;
    return decompressedBytes;
}

void sig_handler(int sigNum){
    if(sigNum == SIGPIPE){
        fprintf(stderr, "SIGPIPE received\n");
        exit(0);
    }
    if(sigNum == SIGINT){
        int val = kill(childpid, SIGINT);
        if(val < 0){
            fprintf(stderr, "Failed to kill process: Error:%d, Message: %s\n", errno, strerror(errno));
            exit(1);
        }
    }
}

void close_pipe_message(int closePipe){
    if(closePipe < 0){
        fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
        exit(1);
    }
}

void fork_process() {
    signal(SIGPIPE, sig_handler);
    signal(SIGINT, sig_handler);
    if(pipe(from_terminal) == -1){
        fprintf(stderr, "Parent to child pipe creation error\n");
        exit(1);
    }
    if(pipe(to_terminal) == -1){
        fprintf(stderr, "child to parent pipe creation error\n");
        exit(1);
    }// help with pipes http://man7.org/linux/man-pages/man2/pipe.2.html
    childpid = fork();
    if(childpid < 0){
        fprintf(stderr, "Fork failed\n");
        exit(1);
    }
    else if(childpid == 0){ //child process (shell)
        int closePipe = close(from_terminal[1]);
        close_pipe_message(closePipe);
        closePipe = close(to_terminal[0]);
        close_pipe_message(closePipe);
        closePipe = dup2(from_terminal[0], STDIN_FILENO); //read-end dup
        close_pipe_message(closePipe);
        closePipe = dup2(to_terminal[1], STDOUT_FILENO); //write-end dup
        close_pipe_message(closePipe);
        closePipe = dup2(to_terminal[1], STDERR_FILENO);
        close_pipe_message(closePipe);
        closePipe = close(from_terminal[0]);
        close_pipe_message(closePipe);
        closePipe = close(to_terminal[1]);
        close_pipe_message(closePipe);
        //finished setting up pipes before execvp
        char* shellName = "/bin/bash";
        char* execArgs[2];
        execArgs[0] = shellName; //no arguments other than its name
        execArgs[1] = NULL;
        int res;
        res = execvp(shellName, execArgs);
        fprintf(stderr, "exec called!");
        if(res == -1){
            fprintf(stderr, "Exec failed \n");
            exit(1);
        }
    }// help with exec from Arpaci book and http://www.csl.mtu.edu/cs4411.ck/www/NOTES/process/fork/exec.html
    else{
        int closePipe = close(from_terminal[0]); //read-end of from terminal pipe
        close_pipe_message(closePipe);
        closePipe = close(to_terminal[1]);
        close_pipe_message(closePipe);
        struct pollfd polls[2];
        polls[0].fd = new_socket_fd;
        polls[0].events = POLLIN | POLLHUP | POLLERR; //keyboard poll
        polls[1].fd = to_terminal[0]; //read from shell
        polls[1].events = POLLIN | POLLHUP | POLLERR;//shell poll
        //used http://pubs.opengroup.org/onlinepubs/009695399/functions/poll.html for poll(2) help
        int val;
        int num;
        int i;
        char compressionBuf[1024];
        char carriage[2];
        carriage[0] = '\r';
        carriage[1] = '\n';
        int exitNum = atexit(print_exit_signal);
        if(exitNum < 0){
            fprintf(stderr, "Error exiting. Error: %d, Message: %s", errno, strerror(errno));
            exit(1);
        }
        if(compressOpt){
            setup_compression();
        }
        for(;;){
            int result = poll(polls, 2, 0);
            getErrors("poll", result, errno);
            if (result > 0){
                if(polls[0].revents & POLLIN){
                    num = read(new_socket_fd, buffer, 1024); //change 1024 later *REMEMBER
                    getErrors("read", num, errno);
                    if(compressOpt){
                        memcpy(compressionBuf, buffer, num);
                        num = decompression(num, 1024, buffer, compressionBuf);
                    }
                    for(i = 0; i < num; i++){
                        char curr = buffer[i];
                        switch(curr){
                            case '\r':
                            case '\n':
                                val = write(from_terminal[1], &carriage[1], sizeof(char));
                                getErrors("write", val, errno);
                                break;
                            case 0x04:
                                val = close(from_terminal[1]);
                                close_pipe_message(val);
                                break;
                            case 0x03:
                                val = kill(childpid, SIGINT);
                                if(val < 0){
                                    fprintf(stderr, "Failed to kill process: Error:%d, Message: %s\n", errno, strerror(errno));
                                    exit(1);
                                }
                                break;
                            default:
                                val = write(from_terminal[1], &curr, sizeof(char));
                                getErrors("write", val, errno);
                                break;
                        }
                    }
                }
                if(polls[0].revents & (POLLHUP | POLLERR)){
                    exit(0);
                }
                if(polls[1].revents & POLLIN){
                    num = read(polls[1].fd, &buffer, 1024);
                    getErrors("read", num, errno);
                    if(compressOpt){
                        memcpy(compressionBuf, buffer, num);
                        num = compression(num, 1024, buffer, compressionBuf);
                    }
                    val = write(new_socket_fd, &buffer, num);
                    getErrors("write", val, errno);
                }
                if(polls[1].revents & (POLLHUP | POLLERR)){
                    exit(0);
                }
            }
        }
    }//parent process (terminal)
}

//help with termios from https://support.sas.com/documentation/onlinedoc/sasc/doc/lr2/tgetatr.htm

void setUpConnection(int portNum){
    socklen_t clientLength;
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if(socketfd < 0){
        fprintf(stderr, "Error with opening the socket\n");
        exit(1);
    }
    memset((char*)&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(portNum);
    server_address.sin_addr.s_addr = INADDR_ANY;
    if(bind(socketfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0){
        fprintf(stderr, "Error with binding: Port in use. \n");
        exit(1);
    }
    listen(socketfd, 5);
    clientLength = sizeof(client_address);
    new_socket_fd = accept(socketfd, (struct sockaddr *) &client_address, &clientLength);
    if(new_socket_fd < 0){
        fprintf(stderr, "Error with accepting\n");
        exit(1);
    }
}

int main(int argc, char** argv){
    int portNum = 0;
    int portOpt = 0;
    static struct option options [] = {
        {"port", 1, 0, 'p'},
        {"compress", 0, 0, 'c'},
        {0, 0, 0, 0}
    };
    int option;
    while((option = getopt_long(argc, argv, "p:c", options, NULL)) != -1){
        switch(option){
            case 'p':
                portNum = atoi(optarg);
                portOpt = 1;
                break;
            case 'c':
                compressOpt = 1;
                break;
            default:
                fprintf(stderr, "Incorrect argument: correct usage is ./lab1b-server --port=portno [--compress] \n");
                exit(1);
        }
    }
    if(!portOpt){
        fprintf(stderr, "Please specify a port number. \n");
        exit(1);
    }
    setUpConnection(portNum);
    fork_process();
    exit(0);
}//server help from http://www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/server.c



