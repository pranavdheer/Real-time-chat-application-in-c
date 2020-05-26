#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>

char prompt[]="Chatroom> ";

/*
get the usage of the script
*/
void usage(){
  printf("-h  print help\n");
  printf("-a  IP address of the server[Required]\n");
  printf("-p  port number of the server[Required]\n");
}


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

int main(int argc, char **argv){
  char *address=NULL,*port=NULL,*username;
  char c;
  //parsing command line arguments
  while((c = getopt(argc, argv, "hu:a:p:")) != EOF){
    switch(c){
      // print help
      case 'h':
         usage();
         break;
      // get server address
      case 'a':
         address=optarg;
         printf("%s\n",address);
         break;
      // get server port number
      case 'p':
         port=optarg;
         printf("%s\n",port);
         break;

      default:
          usage();
          exit(1);

    }


   }


   if(optind  == 1 || port == NULL || address == NULL){
    printf("Invalid usage\n");
    usage();
    exit(1); }

    int connID=connection(address,port);
    
}
