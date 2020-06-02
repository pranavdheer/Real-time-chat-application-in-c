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

#define MAXLINE 1024 /* max line size */

char prompt[]="Chatroom> ";
int flag=0;
/*
get the usage of the script
*/
void usage(){
  printf("-h  print help\n");
  printf("-a  IP address of the server[Required]\n");
  printf("-p  port number of the server[Required]\n");
  printf("-u  enter your username[Required]\n");
}


/*
* @brief-: connects the client to ther sever
* NOTE-: THE function traverses the list to find appropriate socket Connection [ is robust]
* @port-: port number
* @hostname-: ip address of the server
* @return -: connection file descriptor
*/

int connection(char* hostname, char* port){
  int clientfd,rc;
  struct addrinfo hints, *listp, *p;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM; /* Connections only */
  hints.ai_flags |=AI_ADDRCONFIG;
  hints.ai_flags |= AI_NUMERICSERV; //using fixed port number


  if ((rc = getaddrinfo(hostname, port, &hints, &listp)) != 0) {
    fprintf(stderr,"invalid hostname or port number\n");
    return -1;
 }

 for (p = listp; p; p = p->ai_next) {
        /* Create a socket descriptor */
        clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (clientfd < 0) {
            continue; /* Socket failed, try the next */
        }

        /* Connect to the server */
        if (connect(clientfd, p->ai_addr, p->ai_addrlen) != -1) {
            break; /* Success */
        }

        /* Connect failed, try another */
        if (close(clientfd) < 0) {
            fprintf(stderr, "open_clientfd: close failed: %s\n",
                    strerror(errno));
            return -1;
        }
    }

    /* Clean up */
    freeaddrinfo(listp);
    if (!p) { /* All connects failed */
            return -1;
    }
    else { /* The last connect succeeded */
            return clientfd;
    }
}

// read server response
void reader(void* var){
  char buf[MAXLINE];
  rio_t rio;
  int status;
  int connID=(int)var;
  // initialise rio data structure
  rio_readinitb(&rio, connID);
  while(1){
     while((status=rio_readlineb(&rio,buf,MAXLINE)) >0){
          //error
          if(status == -1)
            exit(1);
          if(!strcmp(buf,"\r\n")){
              break;
            }
          // exit from the server
          if (!strcmp(buf,"exit")){
              close(connID);
              exit(0);
            }
          if (!strcmp(buf,"start\n")){

               printf("\n");
            }

          else
             printf("%s",buf);
      }
      // print the Chatroom prompt
      printf("%s",prompt);
      fflush(stdout);
  }
}

int main(int argc, char **argv){


  char *address=NULL,*port=NULL,*username=NULL;
  char cmd[MAXLINE];
  char c;
  pthread_t tid;
  //parsing command line arguments
  while((c = getopt(argc, argv, "hu:a:p:u:")) != EOF){
    switch(c){
      // print help
      case 'h':
         usage();
         exit(1);
         break;
      // get server address
      case 'a':
         address=optarg;
         break;
      // get server port number
      case 'p':
         port=optarg;
         break;
      // get the username
      case 'u':
         username=optarg;
         break;

      default:
          usage();
          exit(1);

    }


   }

   if(optind  == 1 || port == NULL || address == NULL || username == NULL){
    printf("Invalid usage\n");
    usage();
    exit(1); }

    int connID=connection(address,port);
    if(connID == -1){
       printf("Couldn't connect to the server\n");
       exit(1);
    }
    // add a newline
    sprintf(username,"%s\n",username);

    // send the server , your username
    if(rio_writen(connID,username,strlen(username)) == -1){
       perror("not able to send the data");
       close(connID);
       exit(1);
    }

    // a thread for reading server response
    pthread_create(&tid,NULL,reader, (void*)connID);
    // print the Chatroom prompt
    printf("%s",prompt);
    while(1){
      // read the command
      if ((fgets(cmd, MAXLINE, stdin) == NULL) && ferror(stdin)) {
            perror("fgets error");
            close(connID);
            exit(1);
        }


      // send the request to the server
      if (rio_writen(connID,cmd,strlen(cmd)) == -1){
          perror("not able to send the data");
          close(connID);
          exit(1);
        }

    }

}
