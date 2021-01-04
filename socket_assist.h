#ifndef SOCKET_ASSIST_H_
#define SOCKET_ASSIST_H_

#include <stdio.h>  
#include <string.h>   //strlen  
#include <stdlib.h>  
#include <errno.h>  
#include <unistd.h>   //close  
#include <arpa/inet.h>    //inet_pton  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros  
#include <stdbool.h> //bool


#include <pthread.h> 
#include "interface.h"
#include "server_common.h"


// return the master socket file descriptor, -1 for failure
int intiateServerSocket(const int port, struct sockaddr_in* address)
{
    int opt = true;
    int masterSocket;

    //create a master socket  
    if( (masterSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0)   
    {   
        perror("socket failed");   
        return -1;   
    }   
     
    //set master socket to allow multiple connections ,  
    //this is just a good habit, it will work without this  
    if( setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,  sizeof(opt)) < 0 )   
    {   
        perror("setsockopt");   
        return -1;   
    }   
     
    //type of socket created  
    address->sin_family = AF_INET;   
    address->sin_addr.s_addr = INADDR_ANY;   
    address->sin_port = htons(port);   
         
    //bind the socket to localhost LISTENING_PORT  
    if (bind(masterSocket, (struct sockaddr *)address, sizeof(*address))<0)   
    {   
        perror("bind failed");   
        return -1;   
    }   
    printf("Listener on port %d \n", port);   
         
    //try to specify maximum of MAX_N_LISTENING_SOCKETS pending connections for the master socket  
    if (listen(masterSocket, MAX_N_LISTENING_SOCKETS) < 0)   
    {   
        perror("listen");   
        return -1;  
    }

    return masterSocket;
}


#endif // SOCKET_ASSIST_H_