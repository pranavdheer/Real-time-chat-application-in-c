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

struct client{
    char *name;
    int confd;
    struct client *next;
};

struct client *header=NULL;

//**********************************************************************************

/*
 * @brief-: add user to the global user DATA_STRUCTURES
 * INSERTION AN HEAD -> O(1) complexity
*/

void add_user(struct client *user){

   if(header == NULL){
     header=user;
     user->next=NULL;
   }
   else{
      user->next=header;

      header=user;
   }
}
/*
 * @brief-: delete client from thr global list
 *  O(n) complexity
 */
void delete_user(confd){
   struct client *user=header;
   struct client *previous=NULL;
   // identify the user
   while(user->confd!=confd){
     previous=user;
     user=user->next;
   }

   if(previous == NULL)
      header=user->next;

   else
     previous->next=user->next;

   // free the resources
   free(user->name);
   free(user);

}

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
 * send msg to all the clients
 */
void send_msg(int confd,char* msg, char* receiver, char* sender){

    char response[bufsize];
    struct client *user=header;
    if(receiver == NULL)
     while (user != NULL){
      if (user->confd == confd){
         strcpy(response,"msg sent\n\r\n");
         rio_writen(user->confd,response,strlen(response));
       }

      else{
         sprintf(response,"start\n%s:%s\n\r\n",sender,msg);
         rio_writen(user->confd,response,strlen(response));
      }
      user=user->next;
    }
   else{
       while (user != NULL){
         if(!strcmp(user->name,receiver)){
           sprintf(response,"start\n%s:%s\n\r\n",sender,msg);
           rio_writen(user->confd,response,strlen(response));
           strcpy(response,"msg sent\n\r\n");
           rio_writen(confd,response,strlen(response));
           return;
         }
         user=user->next;
       }
        strcpy(response,"user not found\n\r\n");
        rio_writen(confd,response,strlen(response));

   }

}

void evaluate(char *buf ,int confd ,char *username){

  char response[bufsize];
  char msg[bufsize];
  char receiver[bufsize];
  char keyword[bufsize];
  // clear the buffer
  msg[0]='\0';
  receiver[0]='\0';
  keyword[0]='\0';
  struct client *user=header;


  if(!strcmp(buf,"help")){
        sprintf(response,"msg \"text\" : send the msg to all the clients online\n");
        sprintf(response,"%smsg \"text\" user :send the msg to a particular client\n",response);
        sprintf(response,"%sonline : get the username of all the clients online\n",response);
        sprintf(response,"%squit : exit the chatroom\n\r\n",response);
        rio_writen(confd,response,strlen(response));
        return;
   }
   // get the online user name
   if (!strcmp(buf,"online")){
        // empty the buffer
        response[0]='\0';
        //global access should be exclusive
        pthread_mutex_lock(&mutex);
        while(user!=NULL){
        sprintf(response,"%s%s\n",response,user->name);
        user=user->next;

        }
    sprintf(response,"%s\r\n",response);
    //global access should be exclusive
    pthread_mutex_unlock(&mutex);
    rio_writen(confd,response,strlen(response));
    return;
   }

   if (!strcmp(buf,"quit")){
      pthread_mutex_lock(&mutex);
      delete_user(confd);
      pthread_mutex_unlock(&mutex);
      strcpy(response,"exit");
      rio_writen(confd,response,strlen(response));
      close(confd);
      return;

   }

   sscanf(buf,"%s \" %[^\"] \"%s",keyword,msg,receiver);

   if (!strcmp(keyword,"msg")){

        pthread_mutex_lock(&mutex);
        send_msg(confd,msg,receiver,username);
        pthread_mutex_unlock(&mutex);
   }
  else {
     strcpy(response,"Invalid command\n\r\n");
     rio_writen(confd,response,strlen(response));

  }

}
/*
* @brief-: the function handles incoming clients concurrently
* @vargp-: poiner to the connection file descriptor
*/
void* client_handler(void *vargp ){

  char username[bufsize];
  rio_t rio;
  struct client *user;
  long byte_size;
  char buf[bufsize];
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
    // assign space in the global structure
    user=malloc(sizeof(struct client));
    // error handling
    if (user == NULL){
      perror("memory can't be assigned");
      close(confd);
      free(vargp);
      return NULL;
    }
    // user->name=username is not safe
    // as the local stack can be accessed by peer threads
    // assign space in heap
    user->name=malloc(sizeof(username));
    memcpy(user->name,username,strlen(username)+1);
    user->confd=confd;

    //lock
    pthread_mutex_lock(&mutex);
    add_user(user);
    //unlock
    pthread_mutex_unlock(&mutex);

    // read client response
    while((byte_size=rio_readlineb(&rio,buf,bufsize)) >0){

        //strip the newline from the string
        buf[byte_size-1]='\0';
        // take appropriate action
        evaluate(buf,confd,username);

    }

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
