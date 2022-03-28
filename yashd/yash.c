#define _POSIX_SOURCE

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "socket.h"
#include "yash-juan/prompt.h"
#include "yash-juan/utils.h"

int sockfd;
char* user_input;
char yash_output[MAXMSG];


void exit_yash(int signum){
    exit(0);
}

void handleSignal(int signum) {
    char* sig_name;
    if (signum == SIGINT) {
        sig_name = "CTL c\n";
    } else if (signum == SIGTSTP) {
        sig_name = "CTL z\n";
    } else {
        sig_name = "CTL\n";
    }

    write_socket(sockfd, sig_name, NULL);
}

void readStdin() {
    // Change disposition of signal to send message to daemon
    signal(SIGINT, handleSignal);
    signal(SIGTSTP, handleSignal);
    signal(SIGCHLD, exit_yash);

    // Get every user input until interrumpted by an exit signal
    while ((user_input = prompt())) {
        prepend(user_input, "CMD ");
        user_input[strlen(user_input)] = 0;
        if (write_socket(sockfd, user_input, NULL) == -1) {
            printf("Error sending to socket\n");
            exit(-1);
        }
    }
    free(user_input);
    close(sockfd);
    kill(getppid(), 9);
    exit(0);
}

void outputLoop() {
    while(1) {
        memset(yash_output, 0, MAXMSG);
        int n = recv(sockfd, yash_output, MAXMSG, 0);
        if (n < 0) {
            printf("Error recieving from connection\n");
            exit(-1);
        } else if (n == 0) {
            printf("Detected disconnect\n");
            exit(-1);
        } else {
            printf("%s", yash_output);
            fflush(stdout);

        }
    }
}

int main(int argc, char const* argv[]) {
    signal(SIGTSTP, SIG_IGN);
    if (argc != 2) {
        printf("Error: yash requires 1 argument: ");
        printf("yash <IP_Address_of_Server>\n");
        exit(EXIT_FAILURE);
    }
    
    // Initialize the socket with IP address from argv
    sockfd = create_client_socket(argv[1]);
    // Create a child process to continuously read user inputs and 
    // Send them to daemon
    int childpid = fork();
    if (childpid == 0) {
        readStdin();
    } else {
        signal(SIGINT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);

        outputLoop();

        signal(SIGINT, handleSignal);
        signal(SIGTSTP, handleSignal);
    }

    return 0;
}