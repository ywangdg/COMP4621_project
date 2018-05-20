#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <time.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>


#define SERVER_PORT (12345)
#define LISTENNQ (5)
#define MAXLINE (1000)
#define MAXTHREAD (5)

typedef struct {
 char *ext;
 char *mediatype;
} extn;

//Possible media types
extn extensions[] = {
 {"jpg", "image/jpg" },
 {"gz",  "image/gz"  },
 {"html","text/html" },
 {"pdf","application/pdf"},
 {"css", "text/css"},
 {"pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
 {0,0}
};

int threads_count = 0;
char *ROOT;

void* request_func(void *args);

int main(int argc, char **argv)
{
      int listenfd, connfd;

      struct sockaddr_in servaddr, cliaddr;
      socklen_t len = sizeof(struct sockaddr_in);

      char ip_str[INET_ADDRSTRLEN] = {0};
      ROOT = getenv("PWD");

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
  char *reqline[3];
  FILE *file;
  char filename[128] = {0};
  char path[MAXLINE] = {0};
  int index = 0;
  char wrt_buff[MAXLINE] = {0};
  char rcv_buff[MAXLINE] = {0};
  char data_to_send[MAXLINE] = {0};
  int  bytes_rcv,bytes_wrt, total_bytes_wrt, sz, bytes_read, fd;
  char *file_type;
  char content_type[128] = {0};
  char tmp[MAXLINE] = {0};
  char *tmp_ptr;
  char *msg;

  memset(&wrt_buff, 0, sizeof(wrt_buff));
  memset(&rcv_buff, 0, sizeof(rcv_buff));

	printf("heavy computation\n");
	sleep(5);

  /* read the response */
  bytes_rcv = 0;
  while (1) {
          bytes_rcv += recv(connfd, rcv_buff + bytes_rcv, sizeof(rcv_buff) - bytes_rcv - 1, 0);
          /* terminate until read '\n' */
          if (bytes_rcv && rcv_buff[bytes_rcv - 1] == '\n')
                  break;
  }

  /* parse request */
  printf("%s\n", rcv_buff);

  /*parse first line of HTTP request */
  reqline[0] = strtok (rcv_buff, " \t\n");
  printf("%s\n", reqline[0]);
  if ( strncmp(reqline[0], "GET\0", 4)==0 ){
    reqline[1] = strtok (NULL, " \t");
		reqline[2] = strtok (NULL, " \t\n");
    printf("%s\n", reqline[1]);
    printf("%s\n", reqline[2]);
    if ( strncmp( reqline[2], "HTTP/1.0", 8)!=0 && strncmp( reqline[2], "HTTP/1.1", 8)!=0 ){
				write(connfd, "HTTP/1.1 400 Bad Request\n", 25); // send the response header
		}
    else{
      if ( strncmp(reqline[1], "/\0", 2)==0 ){
        reqline[1] = "/index.html";        //Because if no file is specified, index.html will be opened by default (like it happens in APACHE...
      }
      strcpy(path, ROOT);
      strcpy(&path[strlen(ROOT)], reqline[1]);
      printf("file: %s\n", path);

    //
      strncpy(tmp, path, sizeof(tmp));
      tmp_ptr = tmp;
      file_type = strsep(&tmp_ptr, ".");
      file_type = strsep(&tmp_ptr, "");
      printf("%s\n", file_type);
      for (int i = 0; extensions[i].ext != NULL; i++) {
        if (strcmp(file_type, extensions[i].ext) == 0) {
          printf("Opening \"%s\"\n", path);
          strncpy(content_type, extensions[i].mediatype, sizeof(content_type));
          if ( (fd=open(path, O_RDONLY))==-1 ){
            snprintf(wrt_buff, sizeof(wrt_buff) - 1, "HTTP/1.1 404 Not Found\r\nContent-Type: %s\r\nKeep-Alive: timeout=5, max=100\r\nConnection: Keep-Alive\r\n\r\n<html><head><title>404 Not Found</title></head><body><p>404 Not Found: The requested resource %s was not found on this server.</p></body></html>", content_type, path);
            write(connfd, wrt_buff, strlen(wrt_buff));
          }
          else {    //FILE FOUND {
               //ptr = path;
             //printf("%s\n", content_type);
            snprintf(wrt_buff, sizeof(wrt_buff) - 1, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nKeep-Alive: timeout=5, max=100\r\nConnection: Keep-Alive\r\n\r\n", content_type);
            write(connfd, wrt_buff, strlen(wrt_buff));
            while ( (bytes_read=read(fd, data_to_send, MAXLINE))>0 ){
              write (connfd, data_to_send, bytes_read);
            }
          }
          break;
        }
        int size = sizeof(extensions) / sizeof(extensions[0]);
        if (i == size - 2) {
          printf("415 Unsupported Media Type\n");
          snprintf(wrt_buff, sizeof(wrt_buff) - 1,"HTTP/1.1 415 Unsupported Media Type\r\nKeep-Alive: timeout=5, max=100\r\nConnection: Keep-Alive\r\n\r\n<html><head><title>415 Unsupported Media Type</head></title><body><p>415 Unsupported Media Type!</p></body></html>");
          write(connfd, wrt_buff, strlen(wrt_buff));
        //}
        }
      //else{
      //  snprintf(wrt_buff, sizeof(wrt_buff) - 1, "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: %lu\r\nConnection: Keep-Alive\r\n\r\n<html><head><title>404 Not Found</title></head><body><p>404 Not Found: The requested resource %s was not found on this server.</p></body></html>", 159 + strlen(path), path);
      //  write(connfd, wrt_buff, strlen(wrt_buff));
      //}
      }
    }
  }
  else{
    printf("%s\n", "Not a GET request");
    return 0;
  }

  threads_count--;


	close(connfd);
}
