#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <math.h>

#define DEBUG

#include <calcLib.h>

int check_desthost(char *Desthost) {
    struct sockaddr_in sa; //struct for ipv4 
    struct sockaddr_in6 ipv6_sa; //struct for ipv6
    struct addrinfo hints, *res;  //struct for dns data
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; 
    hints.ai_socktype = SOCK_STREAM;

    if (inet_pton(AF_INET, Desthost, &(sa.sin_addr)) == 1) {
        return 1;
    } else if (inet_pton(AF_INET6, Desthost, &(ipv6_sa.sin6_addr)) == 1) {
        return 2;
    } else if (getaddrinfo(Desthost, NULL, &hints, &res) == 0) {
        freeaddrinfo(res);
        return 3;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    char *input = argv[1];
    char *port_no = strrchr(input, ':');
    if (port_no == NULL) {
        printf("Only accepted Input format host:port\n");
        return 0;
    }

    *port_no = '\0'; // Replacing the colon with null terminator

    int sock, port;
    char *Desthost = input;
    char *Destport = port_no + 1; //Get the value at the address after the port_no

    int address_type = check_desthost(Desthost); //Check the address type 

    if (address_type == 0) {  // Check if address type is 0
        printf("Invalid IP address\n");
        return 0;
    }

    port = atoi(Destport);
    printf("Host %s, and port %d.\n", Desthost, port);
#ifdef DEBUG
    printf("Connected to %s:%d and local.\n", Desthost, port);
#endif

    if (address_type == 1) {
        struct sockaddr_in client;
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            perror("Cannot create socket with IPv4 address");
            return 0;
        }

        client.sin_family = AF_INET;
        client.sin_port = htons(port);
        client.sin_addr.s_addr = inet_addr(Desthost);

        if (connect(sock, (struct sockaddr *) &client, sizeof(client)) < 0) {
            perror("Connection failed");
            close(sock);
            return 0;
        }
    } else if (address_type == 2) {
        struct sockaddr_in6 client;
        memset(&client, 0, sizeof(client));

        sock = socket(AF_INET6, SOCK_STREAM, 0);
        if (sock < 0) {
            perror("Cannot create socket with IPv6 address");
            return 0;
        }

        client.sin6_family = AF_INET6;
        client.sin6_port = htons(port);
        if (inet_pton(AF_INET6, Desthost, &client.sin6_addr) != 1) {
            perror("Invalid IPv6 address");
            close(sock);
            return 0;
        }

        if (connect(sock, (struct sockaddr *) &client, sizeof(client)) < 0) {
            perror("Connection failed with the address");
            close(sock);
            return 0;
        }
    } else if (address_type == 3) {
        struct addrinfo hints, *res, *rp;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;    //  IPv4 or IPv6
        hints.ai_socktype = SOCK_STREAM;

        int status = getaddrinfo(Desthost, Destport, &hints, &res);
        if (status != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
            return 0;
        }

        for (rp = res; rp != NULL; rp = rp->ai_next) {
            sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
            if (sock < 0) {
                continue;
            }

            if (connect(sock, rp->ai_addr, rp->ai_addrlen) == 0) {
                printf("Connected to host %s\n", Desthost);
                break;
            }
            close(sock);
        }

        if (rp == NULL) { 
            fprintf(stderr, "Check your DNS name, unable to attach to any address\n");
            freeaddrinfo(res);
            return 0;
        }
        freeaddrinfo(res);
    }

    char buffer[1024], srv_msg[1024];
    char *message = "OK\n";

    // Receive the initial message
    bzero(buffer, sizeof(buffer));
    if (recv(sock, buffer, sizeof(buffer) - 1, 0) < 0) {
        perror("Failed to receive initial message");
        close(sock);
        return 0;
    }
    buffer[sizeof(buffer) - 1] = '\0';
    printf("%s", buffer);

    if (strcmp(buffer, "TEXT TCP 1.0\n\n") == 0) {
        if (send(sock, message, strlen(message), 0) < 0) {
            perror("Failed to send OK message");
            close(sock);
            return 0;
        }
    } else {
        printf("Server repsonse doesn match!: %s\n", buffer);
        close(sock);
        return 0;
    }

    // Receive the operation and operands
    bzero(srv_msg, sizeof(srv_msg));
    if (recv(sock, srv_msg, sizeof(srv_msg) - 1, 0) < 0) {
        perror("Failed to receive operation message");
        close(sock);
        return 0;
    }
    srv_msg[sizeof(srv_msg) - 1] = '\0'; 


   
    char *com[3];
    int j = 0;
    com[j] = strtok(srv_msg, " ");
    while (com[j] != NULL && j < 2) {
        com[++j] = strtok(NULL, " ");
    }

    if (com[0] == NULL || com[1] == NULL || com[2] == NULL) { 
        printf("Invalid operation or operands received.\n");
        close(sock);
        return 0;
    }

    int i1, i2, iresult;
    double f1, f2, fresult;

    char command[10];
    strncpy(command, com[0], sizeof(command) - 1);
    command[sizeof(command) - 1] = '\0';

    char result[256];
    if (command[0] == 'f') {
        f1 = atof(com[1]);
        f2 = atof(com[2]);

        if (strcmp(command, "fadd") == 0) {
            fresult = f1 + f2;
        } else if (strcmp(command, "fmul") == 0) {
            fresult = f1 * f2;
        } else if (strcmp(command, "fsub") == 0) {
            fresult = f1 - f2;
        } else if (strcmp(command, "fdiv") == 0) {
            fresult = f1 / f2;
        } else {
            printf("Unknown command: %s\n", command);
            close(sock);
            return 0;
        }

        printf("%s %8.8g %8.8g\n", command, f1, f2);
        snprintf(result, sizeof(result), "%8.8g\n", fresult);
    } else {
        i1 = atoi(com[1]);
        i2 = atoi(com[2]);

        if (strcmp(command, "add") == 0) {
            iresult = i1 + i2;
        } else if (strcmp(command, "sub") == 0) {
            iresult = i1 - i2;
        } else if (strcmp(command, "div") == 0) {
            iresult = i1 / i2;
        } else if (strcmp(command, "mul") == 0) {
            iresult = i1 * i2;
        } else {
            printf("Unknown command: %s\n", command);
            close(sock);
            return 0;
        }

        printf("Assignment: %s %d %d\n", command, i1, i2 );
        snprintf(result, sizeof(result), "%d\n", iresult);
    }

    if (send(sock, result, strlen(result), 0) < 0) {
        perror("Failed to send result");
        close(sock);
        return 0;
    }

    // 
    bzero(buffer, sizeof(buffer));
    if (recv(sock, buffer, sizeof(buffer), 0) < 0) {
        perror("Failed to receive server reply");
        close(sock);
        return 0;
    }
    printf("Server reply: %s", buffer);

    close(sock);
    return 0;
}
