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

struct termios original_attributes;
struct termios new_attributes;
int from_terminal[2];
int to_terminal[2];
char buffer[1024];
pid_t childpid;
int debugOpt = 0;

void choose_debug(char* s){
    if(strcmp(s, "save") == 0){
        fprintf(stderr, "\r\nsaving terminal attributes\r\n");
    }
    if(strcmp(s, "reset") == 0){
        fprintf(stderr, "\r\nreset terminal atributes\r\n");
    }
    if(strcmp(s, "carriage return") == 0){
        fprintf(stderr, "\r\ncarriage return received!\r\n");
    }
    if(strcmp(s, "^D") == 0){
        fprintf(stderr, "\r\nEOF received!\r\n");
    }
    if(strcmp(s, "^C") == 0){
        fprintf(stderr, "\r\ninterrupt received!\r\n");
    }
    if(strcmp(s, "regular") == 0){
        fprintf(stderr, "\r\nCharacter received!\r\n");
    }
    if(strcmp(s, "term_error") == 0){
        fprintf(stderr, "Error with keyboard!\n");
    }
    if(strcmp(s, "shell_error") == 0){
        fprintf(stderr, "Error with shell!\n");
    }
    if(strcmp(s, "child") == 0){
        fprintf(stderr, "\r\nIn child process!i\r\n");
    }
    if(strcmp(s, "shell_execute") == 0){
        fprintf(stderr, "Shell executed!\n");
    }
    if(strcmp(s, "debug") == 0){
        fprintf(stderr, "Debug option specified!\n");
    }
    if(strcmp(s, "pipes") == 0){
        fprintf(stderr, "\r\nPipes created!\r\n");
    }
    if(strcmp(s, "stdout_and_shell") == 0){
        fprintf(stderr, "\r\nWrite to stdout and shell!\r\n");
    }
    if(strcmp(s, "parent") == 0){
        fprintf(stderr, "\r\nIn parent process!\r\n");
    }
    if(strcmp(s, "shell") == 0){
        fprintf(stderr, "Shell flag specified!\n");
    }
}

void sig_handler(int sigNum){
    if(sigNum == SIGPIPE){
        fprintf(stderr, "SIGPIPE received\n");
        exit(0);
    }
}

void close_pipe_message(int closePipe){
    if(closePipe < 0){
        fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
        exit(1);
    }
}

void poll_err_handle(int isKeyboard){
    int status;
    if(isKeyboard){
        int val = kill(childpid, SIGINT);
        if(val < 0){
            fprintf(stderr, "Error with kill. Error: %d, Message: %s\n", errno, strerror(errno));
            exit(1);
        }
    }
    int res = waitpid(childpid, &status, 0);
    if(res < 0){
        fprintf(stderr, "Error in waitpid: Error: %d, Message: %s\n", errno, strerror(errno));
        exit(1);
    }
    fprintf(stderr, "\r\nSHELL EXIT SIGNAL=%d, STATUS=%d\r\n", WTERMSIG(status), WEXITSTATUS(status));
    //you must close output after
    res = close(to_terminal[0]);
    close_pipe_message(res);
}

void read_write(){
    char curr;
    char tempBuf[2];
    ssize_t num = read(STDIN_FILENO, &curr, sizeof(char));
    if(num < 0){
        fprintf(stderr, "Error reading from input. Error: %d, Message: %s\n", errno, strerror(errno));
        exit(1);
    }
    while(num > 0){
        switch(curr){
            case 0x04:
                if(debugOpt){
                    choose_debug("^D");
                }
                exit(0);
                break;
            case 0x03:
                if(debugOpt){
                    choose_debug("^C");
                }
                break;
            case '\r':
            case '\n':
                if (debugOpt) {
                    choose_debug("carriage return");
                }
                tempBuf[0] = '\r';
                tempBuf[1] = '\n';
                num = write(STDOUT_FILENO, &tempBuf, sizeof(char)*2);
                if(num < 0){
                    fprintf(stderr, "Error writing from input. Error: %d, Message: %s\n", errno, strerror(errno));
                    exit(1);
                }
                break;
            default:
                if(debugOpt){
                    choose_debug("regular");
                }
                num = write(STDOUT_FILENO, &curr, sizeof(char));
                if(num < 0){
                    fprintf(stderr, "Error writing from input. Error: %d, Message: %s\n", errno, strerror(errno));
                    exit(1);
                }
                break;
        }
        num = read(STDIN_FILENO, &curr, sizeof(char));
        if(num < 0){
            fprintf(stderr, "Error reading from input. Error: %d, Message: %s\n", errno, strerror(errno));
            exit(1);
        }
    }
}
void read_input_from_shell(int inputfd){
    ssize_t res = read(inputfd, buffer, 1024);
    int num;
    char tempBuf[2];
    if(res < 0){
        fprintf(stderr, "Error reading from input. Error: %d, Message: %s\n", errno, strerror(errno));
        exit(1);
    }
    int i;
    for(i = 0; i < res; i++){
        char curr = buffer[i];
        switch(curr){
            case '\r':
            case '\n':
                tempBuf[0] = '\r';
                tempBuf[1] = '\n';
                num = write(STDOUT_FILENO, &tempBuf, sizeof(char)*2);
                if(num < 0){
                    fprintf(stderr, "Error writing from input. Error: %d, Message: %s\n", errno, strerror(errno));
                    exit(1);
                }
                break;
            default:
                num = write(STDOUT_FILENO, &curr, sizeof(char));
                if(num < 0){
                    fprintf(stderr, "Error writing from input. Error: %d, Message: %s\n", errno, strerror(errno));
                    exit(1);
                }
                break;
        }
    }
}

void read_input_from_term(int inputfd){
    int i;
    ssize_t ret = read(inputfd, buffer, 1024);
    if(ret < 0){
        fprintf(stderr, "Error reading fron terminal. Error: %d, Message: %s\n", errno, strerror(errno));
        exit(1);
    }
    int res;
    for(i = 0; i < ret; i++){
        char curr = buffer[i];
        char carriage[2];
        carriage[0] = '\r';
        carriage[1] = '\n';
        switch(curr){
            case 0x04:
                res = close(from_terminal[1]);
                close_pipe_message(res);
                break;
                //don't need to close all pipes
            case 0x03:
                res = kill(childpid, SIGINT);
                if(res < 0){
                    fprintf(stderr, "Kill failed. Error: %d, Message: %s\n", errno, strerror(errno));
                    exit(1);
                    break;
                }
            case '\r':
            case '\n'://map <cr> & <lf> to <cr><lf>
                res = write(STDOUT_FILENO, &carriage[0], sizeof(char));
                if(res < 0){
                    fprintf(stderr, "Write failed. Error: %d, Message: %s\n", errno, strerror(errno));
                    exit(1);
                }
                res = write(STDOUT_FILENO, &carriage[1], sizeof(char));
                if(res < 0){
                    fprintf(stderr, "Write failed. Error: %d, Message: %s\n", errno, strerror(errno));
                    exit(1);
                }
                res = write(from_terminal[1], &carriage[1], sizeof(char));
                if(res < 0){
                    fprintf(stderr, "Write failed. Error: %d, Message: %s\n", errno, strerror(errno));
                    exit(1);
                }
                break;
            default:
                res = write(STDOUT_FILENO, &curr, sizeof(char));
                if(res < 0){
                    fprintf(stderr, "Write failed. Error: %d, Message: %s\n", errno, strerror(errno));
                    exit(1);
                }
                    //write to both if it is a shell
                res = write(from_terminal[1], &curr, sizeof(char));
                if(res < 0){
                    fprintf(stderr, "Write failed. Error: %d, Message: %s\n", errno, strerror(errno));
                    exit(1);
                }
                else {
                    if(debugOpt){
                        choose_debug("stdout_and_shell");
                    }
                }
                break;
        }
    }
}

void fork_process() {
    signal(SIGPIPE, sig_handler);
    if(pipe(from_terminal) == -1){
        fprintf(stderr, "Parent to child pipe creation error\n");
        exit(1);
    }
    if(pipe(to_terminal) == -1){
        fprintf(stderr, "child to parent pipe creation error\n");
        exit(1);
    }// help with pipes http://man7.org/linux/man-pages/man2/pipe.2.html
    if(debugOpt){
        choose_debug("pipes");
    }
    childpid = fork();
    if(childpid < 0){
        fprintf(stderr, "Fork failed\n");
        exit(1);
    }
    else if(childpid == 0){ //child process (shell)
        if(debugOpt){
            choose_debug("child");
        }
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
        else{
            if(debugOpt){
                choose_debug("shell_execute");
            }
        }
    }// help with exec from Arpaci book and http://www.csl.mtu.edu/cs4411.ck/www/NOTES/process/fork/exec.html
    else{
        if(debugOpt){
            choose_debug("parent");
        }
        int closePipe = close(from_terminal[0]); //read-end of from terminal pipe
        close_pipe_message(closePipe);
        closePipe = close(to_terminal[1]);
        close_pipe_message(closePipe);
        struct pollfd polls[2];
        polls[0].fd = 0;
        polls[0].events = POLLIN | POLLHUP | POLLERR; //keyboard poll
        polls[1].fd = to_terminal[0]; //read from shell
        polls[1].events = POLLIN | POLLHUP | POLLERR;//shell poll
        //used http://pubs.opengroup.org/onlinepubs/009695399/functions/poll.html for poll(2) help
        for(;;){
            int result = poll(polls, 2, 0);
            if(result < 0){
                fprintf(stderr, "Error in creating poll: Error: %d, Message: %s\n", errno, strerror(errno));
                exit(1);
            }
            else if (result > 0){
                if(polls[0].revents & POLLIN){
                    read_input_from_term(STDIN_FILENO);
                }
                if(polls[0].revents & (POLLHUP | POLLERR)){
                    if(debugOpt){
                        choose_debug("term_error");
                    }
                    poll_err_handle(1);
                    break;
                }
                if(polls[1].revents & POLLIN){
                    read_input_from_shell(polls[1].fd);
                }
                if(polls[1].revents & (POLLHUP | POLLERR)){
                    if(debugOpt){
                        choose_debug("shell_error");
                    }
                    poll_err_handle(0);
                    break;
                }
            }
        }
    }//parent process (terminal)
}

void save_terminal_attributes(){
    if(debugOpt){
        choose_debug("save");
    }
    int result = tcgetattr(STDIN_FILENO, &original_attributes);
    if(result < 0){
        fprintf(stderr, "Error in getting attributes. Error: %d, Message: %s\n", errno, strerror(errno));
        exit(1);
    }
    
}
//help with termios from https://support.sas.com/documentation/onlinedoc/sasc/doc/lr2/tgetatr.htm

void reset(){
    if(debugOpt){
        choose_debug("reset");
    }
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

int main(int argc, char** argv){
    if(isatty(STDIN_FILENO) == 0){ //check if STDIN is a terminal
        fprintf(stderr, "STDIN is not an open file descriptor referring to a terminal. Error: %d, Message: %s\n", errno, strerror(errno));
        exit(1);
    }
    
    static struct option options [] = {
        {"shell", 0, 0, 's'},
        {"debug", 0, 0, 'd'},
        {0, 0, 0, 0}
    };
    int option;
    int shellOpt = 0;
    while((option = getopt_long(argc, argv, "sd", options, NULL)) != -1){
        switch(option){
            case 's':
                shellOpt = 1;
                choose_debug("shell_flag");
                break;
            case 'd':
                debugOpt = 1;
                choose_debug("debug");
                break;
            default:
                fprintf(stderr, "Incorrect argument: correct usage is ./lab1a [--shell] [--debug] \n");
                exit(1);
        }
    }
    set_input_mode();
    if(shellOpt){
        fork_process();
    }
    else {
        //check if it gets read one byte at a time
        read_write();
    }
    exit(0);
}
