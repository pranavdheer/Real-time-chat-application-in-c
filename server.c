#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>


size_t bufsize = 1000;


int connection(char * port){

   struct addrinfo *p, *listp, hints;
   int rc,listenfd;

   //initialise to zero
   memset(&hints,0,sizeof(struct addrinfo));
   hints.ai_family = AF_INET;
   hints.ai_socktype = SOCK_STREAM; /* Connections only */
   hints.ai_flags =AI_ADDRCONFIG|AI_PASSIVE;
   hints.ai_flags |= AI_NUMERICSERV; //using fixed port number


   if ((rc = getaddrinfo(NULL, port, &hints, &listp)) != 0) {
     fprintf(stderr,"get_address info failed port number %s is invalid\n",port);
     return -1;
  }

   // traverse the list of available Connections
   for (p = listp; p; p = p->ai_next) {

       listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
       if (listenfd < 0) {
         continue; /* Socket failed, try the next */
       }
      //bind the socket, returns 0 on Success
      if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) {
          break; /* Success */
      }
      if (close(listenfd) < 0) { /* Bind failed, try the next */
            fprintf(stderr, "open_listenfd close failed: %s\n",
                    strerror(errno));
            return -1;
        }

    }
    printf("%d\n",INADDR_ANY);
    // avoiding memory leak
    freeaddrinfo(listp);
    if (!p) { /* All connects failed */
        return -1;
    }

    // setting backlog to 1024 , desired value
    // set the socket to listen
    if (listen(listenfd, 1024) < 0) {
            close(listenfd);
            return -1;
        }

      return listenfd;
}


int main(){
  struct sockaddr_storage clientaddr;
  socklen_t clientlen;
  int listen=-1;
  char host[1000];
  int confd;


  // get the host name
  if (gethostname(host, sizeof(host)) == -1){
  printf("not able to get the host name\n");
  exit(1);
  }

  // make a connection file descriptor
  listen= connection("80");

  //connection failed
  if(listen == -1){
   printf("connection failed\n");
   exit(1);
  }

  printf("waiting at hostname '%s' and port '%s' \n",host,"80");

  // a client file descriptor
  confd = accept(listen, (struct sockaddr *)&clientaddr, &clientlen);
  printf("client connected\n");
}
