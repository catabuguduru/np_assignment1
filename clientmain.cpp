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


// Included to get the support library
#include <calcLib.h>

int main(int argc, char *argv[]){


	int sock, port;
	char delim[]=":";
	char *Desthost=strtok(argv[1],delim);
	char *Destport=strtok(NULL,delim);
	// *Desthost now points to a sting holding whatever came before the delimiter, ':'.
	// *Dstport points to whatever string came after the delimiter. 

	port=atoi(Destport);
	#ifdef DEBUG 
		printf("Host %s, and port %d.\n",Desthost,port);
	#endif
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		printf("Cannot create socket");
		return 0;
	}

	struct sockaddr_in client;
	client.sin_family = AF_INET;
	client.sin_port = htons(port);
	client.sin_addr.s_addr = inet_addr(Desthost);

	int request = connect(sock, (struct sockaddr *)  &client, sizeof(client));
	if (request < 0) {
		return 0;
	}

	char buffer[1024], srv_msg[1024];
	char *message;
	message = "OK\n";
	if (recv(sock, &buffer, sizeof(buffer), 0) < 0) {
		return 0;
	}
	printf("%s", buffer);
	if (strcmp(buffer, "TEXT TCP 1.0\n\n") == 0) {
		if (send(sock, message, strlen(message), 0) < 0) {
			return 0;
		}
	}
	bzero(srv_msg, sizeof(srv_msg));
	if (recv(sock, &srv_msg, sizeof(srv_msg), 0) < 0) {
		return 0;
	}
	printf("recieved operation and operands are : %s", srv_msg);
	int j = 0;
	char *com[3];
	com[j] = strtok(srv_msg, " ");
	while (com[j] != NULL)
	{
		com[++j] = strtok(NULL, " ");
	}
	int i1, i2, iresult;
	float f1, f2, fresult;

	char command[10];
	strcpy(command, com[0]);
	char result[256];
	if (command[0] == 'f') {
		f1 = atof(com[1]);
		f2 = atof(com[2]);
		if (strcmp(command, "fadd") == 0) {
			fresult = f1 + f2;
		}
		else if (strcmp(command, "fmul") == 0) {
			fresult = f1 * f2;
		}
		else if (strcmp(command, "fsub") == 0) {
			fresult = f1 - f2;
		}
		else if (strcmp(command, "fdiv") == 0) {
			fresult = f1 / f2;
		}
		printf("%s %8.8g %8.8g = %8.8g\n", command, f1, f2, fresult);
		sprintf(result, "%8.8g\n", fresult);
		if (send(sock, result, strlen(result), 0) < 0) {
			return 0;
		}

	}
	else {
		i1 = atoi(com[1]);
		i2 = atoi(com[2]);
		if (strcmp(command, "add") == 0) {
			iresult = i1 + i2;
		}
		else if (strcmp(command, "sub") == 0) {
			iresult = i1 - i2;
		}
		else if (strcmp(command, "div") == 0) {
			iresult = i1 / i2;
		}
		else if (strcmp(command, "mul") == 0) {
			iresult = i1 * i2;
		}
		printf("%s %d %d = %d\n", command, i1, i2, iresult);
		sprintf(result, "%d\n", iresult);
		if (send(sock, result, strlen(result), 0) < 0) {
			return 0;
		}
	}
	char reply[256];
	if (recv(sock, &reply, sizeof(reply), 0) < 0) {
		return 0;
	}
	printf("\n server reply: %s\n", reply);
	close(sock);

	return 0;


  
}
