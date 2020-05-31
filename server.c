#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include "helper.h"

//********************GLOBAL DATA_STRUCTURES & CONSTANTS****************************
#define bufsize 1000

// mutex lock for global data access
pthread_mutex_t mutex;



typedef struct server{
    char



}

//**********************************************************************************




/*
* @brief-: assigns a listning socket at a given port number
* NOTE-: THE function traverses the list to find appropriate socket Connection
* for the server [ isrobust]
* @port-: port number
* @return -: listining file descriptor
*/
int connection(char * port){

   struct addrinfo *p, *listp, hints;
   int rc,listenfd,optval=1;

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

       /* Eliminates "Address already in use" error from bind */
      setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval,sizeof(int));
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

/*
* @brief-: the function handles incoming clients concurrently
* @vargp-: poiner to the connection file descriptor
*/
void* client_handler(void *vargp ){

  char username[bufsize];
  rio_t rio;
  long byte_size;
  // detaching the thread from peers
  // so it no longer needs to be
  // terminated in the main thread
   pthread_detach(pthread_self());

   // saving the connection fd on function stack
   int confd = *((int *)vargp);
   rio_readinitb(&rio, confd);

    // read the user name as a single line , -1 is for error handling
    if( (byte_size=rio_readlineb(&rio,username,bufsize)) == -1){
         close(confd);
         free(vargp);
         return NULL;
    }
    //strip the newline from the string
    username[byte_size-1]='\0';
    // thread lock
    pthread_mutex_lock(&mutex);

    //unlock thread
    pthread_mutex_unlock(&mutex);
    return NULL;
}

int main(int argc,char **argv){
  struct sockaddr_storage clientaddr;
  socklen_t clientlen;
  int listen=-1;
  char host[1000];
  char *port="80";
  int *confd;
  pthread_t tid;

  if (argc > 1)
    port=argv[1];

  // make a connection file descriptor
  listen= connection(port);

  //connection failed
  if(listen == -1){
   printf("connection failed\n");
   exit(1);
  }

  printf("waiting at localhost and port '%s' \n",port);

  // loop to keep accepting clients
  while(1){
      // assign space in the heap [prevents data race]
      confd=malloc(sizeof(int));
      *confd=accept(listen, (struct sockaddr *)&clientaddr, &clientlen);
      printf("A new client is online\n");
      // assign a seperate thread to deal with the new client
       pthread_create(&tid,NULL,client_handler, confd);

  }

}
