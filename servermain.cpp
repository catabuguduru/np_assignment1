#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include "calcLib.c"
#include <calcLib.h>
#include <netdb.h>
#include <sys/select.h>

// Enable if you want debugging to be printed, see examble below.
// Alternative, pass CFLAGS=-DDEBUG to make, make CFLAGS=-DDEBUG
#define DEBUG

int operation(int cnctn){
    char rcv_msg[1450];
    bzero(rcv_msg, sizeof(rcv_msg));
    strcpy(rcv_msg, "TEXT TCP 1.0\n\n");
    if (send(cnctn, rcv_msg, strlen(rcv_msg), 0) < 0) {
        return 0;
    }
    bzero(rcv_msg, sizeof(rcv_msg));
    if (recv(cnctn, &rcv_msg, sizeof(rcv_msg), 0) < 0) {
        return 0;
    }

    char *ptr;
    ptr = randomType();

    double f1, f2, fresult;
    int i1, i2, iresult;
    char reply[1450];
    char client_reply[1450];
    float D;

    if (ptr[0] == 'f') {

        f1 = randomFloat();
        f2 = randomFloat();
        bzero(reply, sizeof(reply));
        sprintf(reply, "%s %8.8g %8.8g\n", ptr, f1, f2);
        if (strcmp(ptr, "fadd") == 0) {
            fresult = f1 + f2;
        }
        else if (strcmp(ptr, "fsub") == 0) {
            fresult = f1 - f2;
        }
        else if (strcmp(ptr, "fmul") == 0) {
            fresult = f1 * f2;
        }
        else if (strcmp(ptr, "fdiv") == 0) {
            fresult = f1 / f2;
        }
        if (send(cnctn, reply, strlen(reply), 0) < 0) {
            return 0;
        }
        if (recv(cnctn, &client_reply, sizeof(client_reply), 0) < 0) {
            return 0;
        }
        double result = atof(client_reply);
        D = abs(result - fresult);
#ifdef DEBUG
    printf("Servers result: %8.8g, Clients result: %8.8g \n ", result, fresult);
#endif
    }
    else {

        i1 = randomInt();
        i2 = randomInt();
        bzero(reply, sizeof(reply));
        sprintf(reply, "%s %d %d\n", ptr, i1, i2);

        if (strcmp(ptr, "add") == 0) {
            iresult = i1 + i2;
        }
        else if (strcmp(ptr, "mul") == 0) {
            iresult = i1 * i2;
        }
        else if (strcmp(ptr, "sub") == 0) {
            iresult = i1 - i2;
        }
        else if (strcmp(ptr, "div") == 0) {
            iresult = i1 / i2;
        }
        if (send(cnctn, reply, strlen(reply), 0) < 0) {
            return 0;
        }
        if (recv(cnctn, &client_reply, sizeof(client_reply), 0) < 0) {
            return 0;
        }
        int result = atof(client_reply);
        D = abs(result - iresult);
    }

    const char *pass = "OK\n";
    const char *fail = "ERROR\n";
  
    if (D < 0.0001) {
        if (send(cnctn, pass, strlen(pass), 0) < 0) {
            return 0;
        }
    }
    else {
        if (send(cnctn, fail, strlen(fail), 0) < 0) {
            return 0;
        }
    }

    return 1;
}

int check_desthost(char *Desthost, struct addrinfo **res){
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(Desthost, NULL, &hints, res) == 0) {
        if ((*res)->ai_family == AF_INET) {
            return 1;
        } else if ((*res)->ai_family == AF_INET6) {
            return 2;
        }
    }
    return 0;
}

int main(int argc, char *argv[]){
    int sock, port;


    char *input = argv[1];
    char *last_colon = strrchr(input, ':');
    if (last_colon == NULL){
        printf("Accepted Input format host:port\n");
        return 0;
    }
    *last_colon = '\0';  // Replace the last colon with a null terminator

    char *Desthost = input;
    char *Destport = last_colon + 1;

    struct addrinfo *res;
    int address_type = check_desthost(Desthost, &res);
    printf("Address type: %d\n", address_type);

    if (address_type == 0){
        printf("Invalid IP address\n");
        return 0;
    } 

    port = atoi(Destport);
#ifdef DEBUG  
    printf("Host: %s, Port: %d\n", Desthost, port);
#endif
    socklen_t cli_len;
    cli_len = sizeof(struct sockaddr_in);

    if (address_type == 1 || address_type == 2){
        if (address_type == 1){
            sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0) {
                printf("Cannot create socket with IPv4 address\n");
                return 0;    
            }
            struct sockaddr_in server;
            memcpy(&server, res->ai_addr, sizeof(server));
            server.sin_port = htons(port);

            if (bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
                printf("Binding failed\n");
                return 0;
            }
        } else if (address_type == 2){
            sock = socket(AF_INET6, SOCK_STREAM, 0);
            if (sock < 0) {
                printf("Cannot create socket with IPv6 address\n");
                return 0;    
            }
            struct sockaddr_in6 server;
            memcpy(&server, res->ai_addr, sizeof(server));
            server.sin6_port = htons(port);

            if (bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
                printf("Binding failed\n");
                return 0;
            }
        }

        listen(sock, 5); // Queue up to 5 clients

        while (1) {
            int cnctn = accept(sock, res->ai_addr, &cli_len);
            if (cnctn < 0) {
                printf("Accept failed\n");
                continue;
            }

            struct timeval timeout;
            timeout.tv_sec = 5;
            timeout.tv_usec = 0;
            setsockopt(cnctn, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

            if (!operation(cnctn)) {
                char error_msg[] = "ERROR TO\n";
                send(cnctn, error_msg, strlen(error_msg), 0);
            }

            close(cnctn);
        }

        close(sock);
    }

    freeaddrinfo(res);
    return 0;
}
