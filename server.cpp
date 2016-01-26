//
// Created by Amanda Jo Fisher on 1/16/16.
//
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define SOCKET_ERROR        -1
#define BUFFER_SIZE         100
#define MESSAGE             "This is the message I'm sending back and forth"
#define QUEUE_SIZE          5

char** str_split(char* a_str, const char* a_delim);

int main(int argc, char* argv[])
{
    int hSocket,hServerSocket;  /* handle to socket */
    struct hostent* pHostInfo;   /* holds info about a machine */
    struct sockaddr_in Address; /* Internet socket address stuct */
    int nAddressSize=sizeof(struct sockaddr_in);
    char pBuffer[BUFFER_SIZE];
    int nHostPort;

    if(argc < 3)
    {
        printf("\nUsage: server host-port dir\n");
        return 0;
    }
    else
    {
        nHostPort=atoi(argv[1]);
    }

    printf("\nStarting server");

    printf("\nMaking socket");
    /* make a socket */
    hServerSocket=socket(AF_INET,SOCK_STREAM,0);

    if(hServerSocket == SOCKET_ERROR)
    {
        printf("\nCould not make a socket\n");
        return 0;
    }

    /* fill address struct */
    Address.sin_addr.s_addr=INADDR_ANY;
    Address.sin_port=htons(nHostPort);
    Address.sin_family=AF_INET;

    printf("\nBinding to port %d",nHostPort);

    /* bind to a port */
    if(bind(hServerSocket,(struct sockaddr*)&Address,sizeof(Address))
       == SOCKET_ERROR)
    {
        printf("\nCould not connect to host\n");
        return 0;
    }
    /*  get port number */
    getsockname( hServerSocket, (struct sockaddr *) &Address,(socklen_t *)&nAddressSize);
    printf(" opened socket as fd (%d) on port (%d) for stream i/o\n",hServerSocket, ntohs(Address.sin_port) );

    printf("Server\n\
              sin_family        = %d\n\
              sin_addr.s_addr   = %d\n\
              sin_port          = %d\n"
            , Address.sin_family
            , Address.sin_addr.s_addr
            , ntohs(Address.sin_port)
    );


    printf("\nMaking a listen queue of %d elements",QUEUE_SIZE);
    /* establish listen queue */
    if(listen(hServerSocket,QUEUE_SIZE) == SOCKET_ERROR)
    {
        printf("\nCould not listen\n");
        return 0;
    }

	int optval = 1;
	setsockopt (hServerSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    for(;;)
    {
            printf("\nWaiting for a connection\n");
            /* get the connected socket */
            hSocket=accept(hServerSocket,(struct sockaddr*)&Address,(socklen_t *)&nAddressSize);

            linger lin;
            unsigned int y=sizeof(lin);
            lin.l_onoff=1;
            lin.l_linger=10;
            setsockopt(hServerSocket,SOL_SOCKET, SO_LINGER,&lin,sizeof(lin));

            printf("\nGot a connection from %X (%d)\n", Address.sin_addr.s_addr, ntohs(Address.sin_port));

            memset(pBuffer,0,sizeof(pBuffer));

        read(hSocket, pBuffer, BUFFER_SIZE);
        printf("Got from the browser: \n%s\n", pBuffer);

        //parse through pBuffer
        const char delim[] = " \f";
        char** request;
        request = str_split(pBuffer, delim);
        printf("%s\n",request[0]); 
        if(strstr(request[0], "GET") != NULL)
        {
            char* address;
            asprintf(&address, "%s%s", argv[2], request[1]);
            printf("absolute path: %s\n", address); 
            int len;
            DIR *dirp;
            struct dirent *dp;
            struct stat filestat;
            int c;
            FILE *file;
            
            printf("Inside first if loop--GET request\n");
            

            if(stat(address, &filestat)) 
            {
                perror("ERROR in stat\n");
                sprintf(pBuffer, "HTTP/1.1 404 FILE NOT FOUND\r\n\r\n%s\n", "<html>Error 404 File Not Found</html>");
                write(hSocket, pBuffer, strlen(pBuffer));
                shutdown(hSocket, SHUT_RDWR);
                if(close(hSocket) == SOCKET_ERROR)
                {
                     printf("\nCould not close socket\n");
                     return 0;
                }
            }
            else if(S_ISREG(filestat.st_mode)) 
            {
                printf("%s is a regular file\n",argv[1]);
                printf("file size = %d\n",(int)filestat.st_size);
                char* headers;
                //jpgs and gif files
                if (strstr(address, ".jpg") || strstr(address, ".gif"))
                {
                    //gif headers
                    if(strstr(address,".gif"))
                    {
                        asprintf(&headers,"Accept-Ranges: bytes\r\n");
                        asprintf(&headers,"%sContent-length: %d\r\n",headers,(int)filestat.st_size);
                        asprintf(&headers,"%sContent-Type: image/gif",headers);
                    }
                    //jpg headers
                    else if(strstr(address,".jpg"))
                    {
                        asprintf(&headers,"Accept-Ranges: bytes\r\n");
                        asprintf(&headers,"%sKeep-Alive: timeout=2, max=100\r\n",headers);
                        asprintf(&headers,"%sContent-length: %d\r\n",headers,(int)filestat.st_size);
                        asprintf(&headers,"%sContent-Type: image/jpeg\r\n",headers);
                        asprintf(&headers,"%sConnection: keep-alive",headers);
                    }
                    FILE *file;
                    file = fopen(address, "rb");
                    char buffer[filestat.st_size]; 
                    int readResult = fread(buffer, 1, filestat.st_size, file);
                    fclose(file); 
                    
                    char *preBody; 
                    asprintf(&preBody, "HTTP/1.1 200 OK\r\n%s\r\n\r\n", headers);
                    
                    write(hSocket, preBody, strlen(preBody));
                    write(hSocket, buffer, sizeof(buffer)); 
                    shutdown(hSocket, SHUT_RDWR);
                if(close(hSocket) == SOCKET_ERROR)
                {
                     printf("\nCould not close socket\n");
                     return 0;
                }
                }
                //html and txt files
                else if (strstr(address, ".html") || strstr(address, ".txt"))
                {
                    //html headers
                    if (strstr(address, ".html"))
                    {
                        asprintf(&headers, "Content-Type: text/html");
                    }
                    //txt headers
                    else
                    {
                        asprintf(&headers, "Accept-Ranges: bytes\r\n");
                        asprintf(&headers,"%sContent-length:%d\r\n",headers,(int)filestat.st_size);
		        		asprintf(&headers,"%sContent-Type: text/plain",headers);
                    }
                    FILE *file;
                    file = fopen(address, "r");
                    char *body;
                    body = (char *)malloc(filestat.st_size); 
                    int i = 0; 
                    int c; 
                    while ((c = getc(file)) != EOF)
                    {
                        body[i]= c;
                        i++;
                    }
                    printf("These are the headers: %s\n", headers); 
                    printf("This is the body: %s\n", body); 
                    
                    char *response;
                    asprintf(&response, "HTTP/1.1 200 OK\r\n%s \r\n\r\n%s", headers, body);
                    printf("This is the response: %s\n", response);
                    write(hSocket, response, strlen(response)); 
                    shutdown(hSocket, SHUT_RDWR);
                if(close(hSocket) == SOCKET_ERROR)
                {
                     printf("\nCould not close socket\n");
                     return 0;
                }
                    fclose(file);
                }
            }	
            else if(S_ISDIR(filestat.st_mode)) 
            {
                printf("Inside directory branch and the address is: %s", address); 
                dirp = opendir(address);
                char* list; 
                asprintf(&list, "<html><ul>"); 
                while ((dp = readdir(dirp)) != NULL)
                {
                    asprintf(&list, "%s<li><a href=\"/%s\">%s</li>\n", list, dp->d_name, dp->d_name); 
                }
                asprintf(&list, "%s</ul></html>", list);
                (void)closedir(dirp);
                char *response;
                asprintf(&response, "HTTP/1.1 200 OK\r\nHeaders\r\n\r\n%s\n", list);
                write(hSocket,response,(strlen(response)));
                /* close socket */
                shutdown(hSocket, SHUT_RDWR);
                if(close(hSocket) == SOCKET_ERROR)
                {
                     printf("\nCould not close socket\n");
                     return 0;
                }
            }	 
        } 	
        else
        {
            printf("404 Error"); 
            sprintf(pBuffer, "HTTP/1.1 400 BAD REQUEST\r\nHeaders\r\n\r\n%s\n", "<html>Error 400 Bad Request</html>");
        }
    }
}

char** str_split(char* a_str, const char* a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (*a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }
    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);
    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;
    result = (char **)malloc(sizeof(char*) * count);
    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, a_delim);
        while (token)
        {
            //assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, a_delim);
        }
        //assert(idx == count - 1);
        *(result + idx) = 0;
    }
    return result;
}
