/*
 * Developed by Mehdi Mohammadi
 * Western Michigan University
 * Computer Science Department 
 * July 15, 2014
 */

#include <arpa/inet.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>

#define MAXLINE 2048
#define MAXSUB  2000

extern int h_errno;

ssize_t post_data(int sockfd, char *host, char *poststr)
{
	char postdata[MAXLINE + 1], recvdata[MAXLINE + 1];
	ssize_t n;
	snprintf(postdata, MAXSUB,
		 "POST /service/v2/value HTTP/1.1\r\n"
		 "Host: %s\r\n"
		 "Content-type: application/x-www-form-urlencoded\r\n"
		 "Content-length: %d\r\n\r\n"
		 "%s", host, strlen(poststr), poststr);

	write(sockfd, postdata, strlen(postdata));
	if ((n = read(sockfd, recvdata, MAXLINE)) > 0) {
		recvdata[n] = '\0';
		printf("%s", recvdata);
	}
	return n;
}

void delay(int ms)
{
    long pause;
    clock_t now,then;

    pause = ms*(CLOCKS_PER_SEC/1000);
    now = then = clock();
    while( (now-then) < pause )
        now = clock();
}

int main(void)
{
	int sockfd;
	struct sockaddr_in servaddr;

	char **pptr;
	char *cloud_host = "nimbits-02.appspot.com"; // Nimbits' public cloud
	char *email = "youremail@gmail.com";
	char *key= "yourKey";

	char pstr[200];
	char *poststr = "email=%s&key=%s&id=%s/tempreture&json={\"d\":%d}\r\n";
	

	char str[50];
	struct hostent *hptr;
	if ((hptr = gethostbyname(cloud_host)) == NULL) {
		fprintf(stderr, " gethostbyname error for host: %s: %s",
			cloud_host, hstrerror(h_errno));
		exit(1);
	}

	if (hptr->h_addrtype == AF_INET
	    && (pptr = hptr->h_addr_list) != NULL) {
		printf("address: %s\n",
		       inet_ntop(hptr->h_addrtype, *pptr, str,
				 sizeof(str)));
	} else {
		fprintf(stderr, "Error call inet_ntop \n");
	}
	time_t t;
	srand((unsigned) time(&t));
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(80); // http port number
	inet_pton(AF_INET, str, &servaddr.sin_addr);

	printf("connecting to server...\n");
	connect(sockfd, (struct sockaddr *) & servaddr, sizeof(servaddr));
	printf("connected...\n");
  	int j = 1;
	// send several random generated numbers as tempratures
	for(j=1; j<=10; j++) {
	  int d = rand();
	  d = d % 100;
	  sprintf(pstr, poststr, email, key, email, d);
	  post_data(sockfd, cloud_host, pstr);
	  printf("data sent...\n");
	  delay(3000);
	}
	close(sockfd);

	exit(0);

}
