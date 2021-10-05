#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "calcLib.c"
#include <calcLib.h>

// Enable if you want debugging to be printed, see examble below.
// Alternative, pass CFLAGS=-DDEBUG to make, make CFLAGS=-DDEBUG
#define DEBUG


using namespace std;


int main(int argc, char *argv[]){
  
	int sock,request,port;
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

	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = inet_addr(Desthost);

	request = bind(sock, (struct sockaddr *) &server, sizeof(server));
	if (request < 0) {
		return 0;
	}

	listen(sock, 10);
	int cnctn;
	initCalcLib();
	while (1) {
		socklen_t cli_len;
		cli_len = sizeof(struct sockaddr_in);
		cnctn = accept(sock, (struct sockaddr *) &server, &cli_len);
		char rcv_msg[256];
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
		char reply[256];
		char srv_result[256];
		char client_reply[256];
		float result, D;

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
			sprintf(srv_result, "%8.8g\n", fresult);
			if (send(cnctn, reply, strlen(reply), 0) < 0) {
				return 0;
			}
			if (recv(cnctn, &client_reply, sizeof(client_reply), 0) < 0) {
				return 0;
			}
			result = atof(client_reply);
			D = abs(result - fresult);

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
			sprintf(srv_result, "%d\n", iresult);
			if (send(cnctn, reply, strlen(reply), 0) < 0) {
				return 0;
			}
			if (recv(cnctn, &client_reply, sizeof(client_reply), 0) < 0) {
				return 0;
			}
			float result, D;
			result = atof(client_reply);
			D = abs(result - iresult);
		}

		char *pass;
		char *fail;
		pass = "OK\n";
		fail = "error\n";
		if (D < 0.0001) {
			if (send(cnctn, pass, sizeof(pass), 0) < 0) {
				return 0;
			}
		}
		else {
			if (send(cnctn, fail, sizeof(fail), 0) < 0) {
				return 0;
			}
		}
		close(cnctn);
	}

	close(sock);


}
