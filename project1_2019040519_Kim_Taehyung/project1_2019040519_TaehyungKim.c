#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024

void error(char *msg) {
    perror(msg);
    exit(1);
}              
                                                                                              
// This function returns the content type requested by the client
char *get_cntType(char* content){
    char *ext = strrchr(content, '.');
    if(ext){
        if(!strcmp(ext, ".html"))
            return "text/html";
        else if(!strcmp(ext, ".gif"))
            return "image/gif";
        else if(!strcmp(ext, ".jpeg"))
            return "image/jpeg";
        else if(!strcmp(ext, ".mp3"))
            return "audio/mp3";
        else if(!strcmp(ext, ".pdf"))
            return "application/pdf";
        else
            return "application/octet-stream";
    }
    else 
        return "application/octet-stream";
}

int main(int argc, char **argv) {
    int sockfd;  // Server socket
    int newsockfd;  // Client socket
    char request[BUFFER_SIZE]; // Request message buffer
    char response[BUFFER_SIZE]; // Response message buffer
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen = sizeof(cli_addr);

    if(argc < 2) {
         fprintf(stderr, "ERROR, no port provided\n");
         exit(1);
     }

    // Make TCP socket
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        error("Opening Socket ERROR");
    }

    // Initialize server address information
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;                 
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    serv_addr.sin_port = htons(atoi(argv[1]));     

    // Enables reallocation of port in time-wait state
    int option = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    
    // Assign server address 
    if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        error("Binding ERROR");
    }

    // Listen for socket connections, backlog = 5
    if(listen(sockfd, 5) == -1) {
        error("Listening ERROR");
    }
    while(1) {
        printf("Waiting for client request ... \n");
        // Accept client request 
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if(newsockfd < 0){
            error("Accepting ERROR");
        }
        memset(&request, 0, BUFFER_SIZE);
        memset(&response, 0, BUFFER_SIZE);
        // Read client request message
        if(read(newsockfd, request, BUFFER_SIZE) < 0){
            error("Reading Socket ERROR");
        }
        printf("\n\n[Request Message]\n%s\n", request);

        int fd; // file descriptor
        int cntLen; // content length
        // Parse client request message
        char *method = strtok(request, " "); // Request method
        char *content = strtok(NULL, " "); // Request content
        
        if(method && content){
            if(!strcmp(content, "/")){ 
                strcpy(content, "/index.html");
            }
            char *local_path = content + 1;
            char *cntType = get_cntType(local_path);
            fd = open(local_path, O_RDONLY);
            if(fd < 0){ // 404 Error
                fd = open("./404.html", O_RDONLY); 
                cntLen = lseek(fd, 0, SEEK_END); // Calculate the length of 404.html file
                // Fill the response buffer with 404 error message
                sprintf(response, "HTTP/1.1 404 Not Found\r\nServer:Linux Web Server\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", cntLen); 
            }
            else { // 200 OK
                cntLen = lseek(fd, 0, SEEK_END); // Calculate the length of request file
                // Fill the response buffer with 200 OK message
                sprintf(response, "HTTP/1.1 200 OK\r\nServer:Linux Web Server\r\nContent-Length: %d\r\nContent-Type: %s\r\n\r\n", cntLen, cntType);
            }
        } 
        else { // 400 Error
            fd = open("./400.html", O_RDONLY);
            cntLen = lseek(fd, 0, SEEK_END); // Calculate the length of 400.html file
            // Fill the response buffer with 400 error message
            sprintf(response, "HTTP/1.1 400 Bad Request\r\nServer:Linux Web Server\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", cntLen);
        }
        lseek(fd, 0, SEEK_SET); // Reset cursor
        printf("\n[Response Message]\n%s\n", response);
        // Write response message to the client socket
        write(newsockfd, response, strlen(response));
        while (read(fd, request, BUFFER_SIZE) > 0)
            write(newsockfd, request, BUFFER_SIZE);
        close(fd);
        close(newsockfd);
    }
    close(sockfd);
    return 0;
}



