/*
*server.c built off of multithreaded echo server template provided by:
*http://www.csc.villanova.edu/~mdamian/sockets/echoC.htm
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>      /* for fgets */
#include <strings.h>     /* for bzero, bcopy */
#include <unistd.h>      /* for read, write */
#include <sys/socket.h>  /* for socket use */
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>
#include <linux/limits.h>

#define MAXLINE  8192  /* max text line length */
#define MAXBUF   8192  /* max I/O buffer size */
#define LISTENQ  1024  /* second argument to listen() */

/* 
 * open_listenfd - open and return a listening socket on port
 * Returns -1 in case of failure 
 */
int open_listenfd(int port); //http://www.csc.villanova.edu/~mdamian/sockets/echoC.htm
void * thread(void *vargp); // http://www.csc.villanova.edu/~mdamian/sockets/echoC.htm
void echo(int connfd);
void sighandler(int); // https://www.tutorialspoint.com/c_standard_library/c_function_signal.htm

static volatile int run = 1; //for handling interupts


int main (int argc, char **argv) {
   int listenfd, *connfdp, port, clientlen=sizeof(struct sockaddr_in);
   struct sockaddr_in clientaddr;
   pthread_t tid; 

   signal(SIGINT, sighandler);
   //make sure program was run with correct # of arguments
   if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(0);
   }
   port = atoi(argv[1]);
   listenfd = open_listenfd(port); // set up tcp port
   printf("starting server\n");
   while(run) {
    //printf("Going to sleep for a second...\n");
    //sleep(1);
    connfdp = malloc(sizeof(int)); //I never free this but neither does the example template. 
	*connfdp = accept(listenfd, (struct sockaddr*)&clientaddr, &clientlen);
    pthread_create(&tid, NULL, thread, connfdp);
   }
   return(0);
}
//http://www.csc.villanova.edu/~mdamian/sockets/echoC.htm
void * thread(void * vargp) 
{  
    //printf("entering thread handler\n");
    int connfd = *((int *)vargp);
    pthread_detach(pthread_self()); 
    free(vargp);
    //printf("entering echo\n");
    echo(connfd);
    close(connfd);
    return NULL;
}


void sighandler(int signum) { //exit gracefully
   
   printf("Caught interupt signal, closing server now");
   run = 0;
   exit(1);
}

void echo (int connfd) {
    char error400msg[] = "HTTP/1.1 400 Bad Request\r\nContent-Type:text/plain\r\nContent-Length:0\r\n\r\n";
    char error403msg[] = "%s 403 Forbidden\r\nContent-Type:text/plain\r\nContent-Length:0\r\n\r\n";
    char error404msg[] = "%s 404 Not Found\r\nContent-Type:text/plain\r\nContent-Length:0\r\n\r\n";
    char error405msg[] = "%s 405 Method Not Allowed\r\nContent-Type:text/plain\r\nContent-Length:0\r\n\r\n";
    char error505msg[] = "HTTP/1.1 505 HTTP Version Not Supported\r\nContent-Type:text/plain\r\nContent-Length:0\r\n\r\n";
    char sendBuf[MAXBUF];
    char receiveBuf[MAXBUF];
    FILE *file;
    size_t bytesCopied;
    long fSize;
    char *tokptr;
    char *method;
    char *version;
    char *uri;
    char *type;
    char fPath[PATH_MAX];

    //char *wwwPath = realpath("./www/", NULL);

	bzero(receiveBuf, MAXBUF);
    bzero(sendBuf, MAXBUF);
    printf("reading socket\n");

    read(connfd, receiveBuf, MAXBUF);
    printf("received the following packet:\n%s", receiveBuf);

    printf("\n--parsing request--\n");

    method = strtok_r(receiveBuf, " ", &tokptr);
    //printf("got token");
    //strncpy(method, tok1, strlen(tok1));
    //printf("method:%s\n", method);

    uri = strtok_r(NULL, " ", &tokptr);
    //strncpy(uri, tok2, strlen(tok2));
    //printf("uri:%s\n", uri);
    version = strtok_r(NULL, "\r\n", &tokptr);
    //strncpy(version, tok3, strlen(tok3));
    //printf("version:%s\n", version);

    //printf("checking if method/uri/version exists\n");
    if(method && uri && version){
        //printf("checking method\n");
    if (strcmp(method, "GET") == 0) {
        //printf("checking versiont\n");
        if (strcmp(version, "HTTP/1.1") == 0 || strcmp(version, "HTTP/1.0") == 0) {
            //printf("parsing file: uri length = %ld\n", strlen(uri));
            //check if default / or move pointer to next char in string so you use relative path
            if(uri[0] == '/' && strlen(uri) == 1) {
                //printf("in if statement\n");
                uri = "index.html"; // default webpage
            }
            else {
                uri++;
            }
            type = strrchr(uri, '.');
            //printf("parsing type\n");
            if (strcmp(type,".html") == 0) {
                type = "text/html";
            }
            else if (strcmp(type, ".txt") == 0) {
                type = "text/plain";
            }
            else if (strcmp(type, ".png") == 0) {
                type = "image/png";
            }
            else if (strcmp(type, ".gif") == 0) {
                type = "image/gif";
            }
            else if (strcmp(type, ".jpg") == 0) {
                type = "image/jpg";
            }
            else if (strcmp(type, ".css") == 0) {
                type = "text/css";
            }
            else if (strcmp(type, ".js") == 0) {
                type = "application/javascript";
            }
            else {
                type = NULL;
            }
            printf("Filename:%s\nType:%s\n", uri, type);
            strcpy(fPath,"www/");
            strcat(fPath, uri);

            if (type != NULL) {
                //printf("checking if file is valid\n");
                file = fopen(fPath,"r");
                if (file > 0) {
                    fseek(file, 0L, SEEK_END);
				    fSize = ftell(file);
				    fseek(file, 0L, SEEK_SET);
                    //printf("sending header\n");
                    sprintf(sendBuf, "%s %d Document Follows\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n", version, 200, type, fSize);
                    send(connfd, sendBuf, strlen(sendBuf), 0);
                    //printf("sending rest of data\n");
                    do {
                        bzero(sendBuf, MAXBUF);

                        bytesCopied = fread(sendBuf, 1, MAXBUF, file);

                        //send(connfd, sendBuf, bytesCopied, 0);
                        if (send(connfd, sendBuf, bytesCopied, 0) == -1) {
                            perror("send Failed");
                            break;
                        }
                    }while(bytesCopied == MAXBUF);
                    //printf("data sent\n\n");

                    bzero(sendBuf, MAXBUF);
                    fclose(file);

                }
                else {
                    if (access(uri, R_OK) !=0 && access(uri, F_OK) == 0) {
                        sprintf(sendBuf, error403msg, version);
                    }
                    else {
                        sprintf(sendBuf,error404msg, "HTTP/1.1");
                    }
                }

            }
            else {
                sprintf(sendBuf,error404msg, version);
            }
        }
        else {
            sprintf(sendBuf,error505msg);
        }
    }
    else {
        if (version) {
            sprintf(sendBuf,error405msg,version);
        }
        else {
            sprintf(sendBuf,error405msg,version);
        }
    }
    }
    else {
        sprintf(sendBuf,error400msg);
    }

    if (strlen(sendBuf) > 0){
		send(connfd, sendBuf, strlen(sendBuf), 0);
	}
}

/* 
 * http://www.csc.villanova.edu/~mdamian/sockets/echoC.htm
 * open_listenfd - open and return a listening socket on port
 * Returns -1 in case of failure 
 */
int open_listenfd(int port) 
{
    int listenfd, optval=1;
    struct sockaddr_in serveraddr;
  
    /* Create a socket descriptor */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    /* Eliminates "Address already in use" error from bind. */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, 
                   (const void *)&optval , sizeof(int)) < 0)
        return -1;

    /* listenfd will be an endpoint for all requests to port
       on any IP address for this host */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET; 
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    serveraddr.sin_port = htons((unsigned short)port); 
    if (bind(listenfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0)
        return -1;

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, LISTENQ) < 0)
        return -1;
    return listenfd;
} /* end open_listenfd */