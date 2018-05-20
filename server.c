#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <time.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <unistd.h>


#define SERVER_PORT (12345)
#define LISTENNQ (5)
#define MAXLINE (1000)
#define MAXTHREAD (5)

void* request_func(void *args);

int main(int argc, char **argv)
{
      int listenfd, connfd;

      struct sockaddr_in servaddr, cliaddr;
      socklen_t len = sizeof(struct sockaddr_in);

      char ip_str[INET_ADDRSTRLEN] = {0};


      int threads_count = 0;
      pthread_t threads[MAXTHREAD];


      /* initialize server socket */
      listenfd = socket(AF_INET, SOCK_STREAM, 0); /* SOCK_STREAM : TCP */
      if (listenfd < 0) {
              printf("Error: init socket\n");
              return 0;
      }

      /* initialize server address (IP:port) */
      memset(&servaddr, 0, sizeof(servaddr));
      servaddr.sin_family = AF_INET;
      servaddr.sin_addr.s_addr = INADDR_ANY; /* IP address: 0.0.0.0 */
      servaddr.sin_port = htons(SERVER_PORT); /* port number */

      /* bind the socket to the server address */
      if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr)) < 0) {
              printf("Error: bind\n");
              return 0;
      }

      if (listen(listenfd, LISTENNQ) < 0) {
              printf("Error: listen\n");
              return 0;
      }

      /* keep processing incoming requests */
      while (1) {
              /* accept an incoming connection from the remote side */
              connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &len);
              if (connfd < 0) {
                      printf("Error: accept\n");
                      return 0;
              }
              memset(&ip_str, 0, sizeof(ip_str));

              /* print client (remote side) address (IP : port) */
              inet_ntop(AF_INET, &(cliaddr.sin_addr), ip_str, INET_ADDRSTRLEN);
              printf("Incoming connection from %s : %hu with fd: %d\n", ip_str, ntohs(cliaddr.sin_port), connfd);

              /* create dedicate thread to process the request */
              if (pthread_create(&threads[threads_count], NULL, request_func, (void *)(intptr_t)connfd) != 0) {
		              printf("Error when creating thread %d\n", threads_count);
		              return 0;
              }
              if (++threads_count >= MAXTHREAD) {
		              break;
	            }

        }
        printf("Nax thread number reached, wait for all threads to finish and exit...\n");
        for (int i = 0; i < MAXTHREAD; ++i) {
          pthread_join(threads[i], NULL);
        }
        return 0;
}

void* request_func(void *args)
{
	/* get the connection fd */
	int connfd = (intptr_t)args;
  char buff[MAXLINE] = {0};
  FILE *file;
  char filename[128] = {0};
  int index = 0;
  char wrt_buff[MAXLINE] = {0};
  char rcv_buff[MAXLINE] = {0};
  int  bytes_rcv,bytes_wrt, total_bytes_wrt;

  memset(&wrt_buff, 0, sizeof(wrt_buff));
  memset(&rcv_buff, 0, sizeof(rcv_buff));

	printf("heavy computation\n");
	sleep(10);

  /* read the response */
  bytes_rcv = 0;
  while (1) {
          bytes_rcv += recv(connfd, rcv_buff + bytes_rcv, sizeof(rcv_buff) - bytes_rcv - 1, 0);
          /* terminate until read '\n' */
          if (bytes_rcv && rcv_buff[bytes_rcv - 1] == '\n')
                  break;
  }

  printf("%s\n", rcv_buff);

  strncpy(filename, "index.html", sizeof(filename));
//  filename = "/index.html";

  /* parse request */

  // sscanf(rcv_buff, "%s\n", filename);

  /* read from file */
  printf("Reading from file '%s' ...\n", filename);
  //
  file = fopen(filename, "r");
  if (!file) {
                snprintf(wrt_buff, sizeof(wrt_buff) - 1, "%s\n", "The file your requested does not exist ...");
  } else {
    while ((wrt_buff[index] = fgetc(file)) != EOF) {
       ++index;
       }
     fclose (file);
  }
  // /* prepare for the send buffer */
  // snprintf(buff, sizeof(buff) - 1, "This is the content sent to connection %d\r\n", connfd);
  //
	// /* write the buffer to the connection */
	// write(connfd, buff, strlen(buff));

  /* send response */
  bytes_wrt = 0;
  total_bytes_wrt = strlen(wrt_buff);
  while (bytes_wrt < total_bytes_wrt) {
          bytes_wrt += write(connfd, wrt_buff + bytes_wrt, total_bytes_wrt - bytes_wrt);
  }


	close(connfd);
}
