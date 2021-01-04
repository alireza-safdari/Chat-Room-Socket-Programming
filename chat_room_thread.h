#ifndef CHAT_ROOM_THREAD_H_
#define CHAT_ROOM_THREAD_H_


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
#include "socket_assist.h"
#include "room_assist.h"


// chat room to be ran as a thread
void *chatRoomRunTimeThread(void *chatRoom) 
{ 
    Room * const room_ = chatRoom;
    int port_ = room_->portN;
    int masterSocket_;
    struct sockaddr_in address_;

    masterSocket_ = intiateServerSocket(port_, &address_);
    if (masterSocket_ == -1)
    {
        room_->roomStatus = THREAD_CRASHED;
        return NULL;
    }

    // running chat mode, since socket is initiated
    room_->roomStatus = RUNNING_CHAT_ROOM;

    //accept the incoming connection   
    printf("Thread \"%s\": Waiting for connections ...\n", room_->roomName);   

    int addressLength = sizeof(address_); // it is used in the while and does not change its value


    while(1)   
    {
        int i; // for the iterations in loops

        fd_set roomFds;
        //clear the socket set  
        FD_ZERO(&roomFds);  
     
        if(room_->roomStatus == DELETE_CHAT_ROOM) // room has to be deleted
        {
            break;
        }

        //add master socket to set  
        FD_SET(masterSocket_, &roomFds);   
        int maxFileDescriptor = masterSocket_;   
             
        //add child sockets to set  
        for (i = 0; i < MAX_N_MEMBERS_PER_ROOM; i++)   
        {   
            //socket descriptor  
            int socketDescriptor = room_->membersSocket[i];   
                 
            //if valid socket descriptor then add to read list  
            if(socketDescriptor > 0)
            {
                FD_SET(socketDescriptor, &roomFds);   
                //highest file descriptor number, need it for the select function  
                if(socketDescriptor > maxFileDescriptor)
                    maxFileDescriptor = socketDescriptor;
            }
        }   
         
        struct timeval timeOut = {0, 100000}; // 100ms
        //wait for an activity on one of the sockets , timeout is 100ms
        // this time out is needed, to make sure delete requests are handled too
        // the main chat thread does not care about this threads being stopped or not. It only considers the roomStatus so 100ms would not be a problem 
        int activity = select(maxFileDescriptor + 1, &roomFds, NULL, NULL, &timeOut);   
        
        // check to see if there is an activity
        if (activity > 0)
        {     
            //If something happened on the master socket,  
            //then its an incoming connection  
            if (FD_ISSET(masterSocket_, &roomFds))   
            {
                // check if there available space for a new member to join
                if (room_->nMembers < MAX_N_MEMBERS_PER_ROOM)
                {
                    int newSocket;
                    if ((newSocket = accept(masterSocket_,  (struct sockaddr *)&address_, (socklen_t*)&addressLength)) < 0)
                    {
                        room_->roomStatus = THREAD_CRASHED;
                        return NULL;
                    }

                    // new connection IP and port
                    printf("T \"%s\": new user connected.   ip: %s  |  port: %d  |  #Members is: %d\n", 
                            room_->roomName, inet_ntoa(address_.sin_addr), ntohs (address_.sin_port), room_->nMembers);


                    // finding an available socket fd storage
                    for (i = 0; i < MAX_N_MEMBERS_PER_ROOM; i++)   
                    {
                        //checl if socket fd storage is available
                        if (room_->membersSocket[i] == 0)
                        {
                            room_->membersSocket[i] = newSocket;
                            room_->nMembers++;
                            break;
                        }
                    }
                }
                else
                {
                    printf("T \"%s\": Room is full.   #Members is:%d\n", room_->roomName, room_->nMembers);
                }
            }   
                
            //checke IO operation on member socket 
            for (i = 0; i < MAX_N_MEMBERS_PER_ROOM; i++)   
            {   
                int socketDescriptor = room_->membersSocket[i];   
                if ( socketDescriptor != 0) // check if it is set to fd
                {      
                    if (FD_ISSET(socketDescriptor, &roomFds)) //  check if this fd has had an activity
                    {   
                        // Check if it was for closing , and also read the incoming message  
                        char message[MAX_DATA + 1];  // +1 for null terminated string so that %s works

                        int messageSize = read( socketDescriptor, message, MAX_DATA); // read possible incoming messages
                        if (messageSize == 0)   
                        {   
                            // Somebody disconnected , get his details  
                            getpeername(socketDescriptor, (struct sockaddr*)&address_, (socklen_t*)&addressLength);
                            
                            // Close the socket and mark as 0 in list for reuse  
                            close( socketDescriptor );   
                            
                            room_->membersSocket[i] = 0;
                            room_->nMembers --; // one member is removed  
                            
                            printf("T \"%s\": member disconnected.   ip: %s  |  port: %d  |  #Members is: %d\n", 
                                    room_->roomName, inet_ntoa(address_.sin_addr), ntohs (address_.sin_port), room_->nMembers);
 
                        }    
                        else // we have gotten a message, we need to send the message tto other members
                        {
                            int j;

                            // get sender details 
                            getpeername(socketDescriptor, (struct sockaddr*)&address_, (socklen_t*)&addressLength);
                            // successfully received
                            printf("T \"%s\": received.  message: \"%.*s\"   ip: %s  |  port: %d\n", 
                            room_->roomName, messageSize - 1, message, inet_ntoa(address_.sin_addr), ntohs (address_.sin_port));
                            // messageSize - 1 because enter make the message ugly

                            // finding other members
                            for (j = 0; j < MAX_N_MEMBERS_PER_ROOM; j++)
                            {
                                int otherMemberFd = room_->membersSocket[j];
                                if (otherMemberFd != 0) // checking if the Fd is valid
                                {
                                    // we do not want to send to the same member
                                    if (socketDescriptor != otherMemberFd)
                                    {
                                        if (send(otherMemberFd, message, messageSize, 0)  != -1)
                                        {
                                            // get member details 
                                            getpeername(socketDescriptor, (struct sockaddr*)&address_, (socklen_t*)&addressLength);
                                            // successfully sent
                                            printf("T \"%s\": sent.  message: \"%.*s\"   ip: %s  |  port: %d\n", 
                                                room_->roomName, messageSize - 1, message, inet_ntoa(address_.sin_addr), ntohs (address_.sin_port));
                                                // messageSize - 1 because enter make the message ugly
                                        }
                                        else
                                        {
                                            // failed to send
                                            // get member details  
                                            getpeername(socketDescriptor, (struct sockaddr*)&address_, (socklen_t*)&addressLength);
                                            // successfully sent
                                            printf("T \"%s\": failed sending.  message: \"%.*s\"   ip: %s  |  port: %d\n", 
                                                room_->roomName, messageSize - 1, message, inet_ntoa(address_.sin_addr), ntohs (address_.sin_port));
                                                // messageSize - 1 because enter make the message ugly
                                        }                            
                                    }
                                }
                            }        
                        }       
                    }   
                }   
            }
        } // end of checking for activity   
    } // end of while(1)

    // give warning to members
    int i;
    for (i = 0; i < MAX_N_MEMBERS_PER_ROOM; i++)
    {
        // finding chatroom members
        int socketDescriptor = room_->membersSocket[i];
        if (socketDescriptor != 0) // checkign if fd is valid
        {
            char message[] = "Warnning: the chatting room is going to be closed...\n";
            int messageSize = strlen(message);

            send(socketDescriptor, message, messageSize, 0 ); // sending warning
            close(socketDescriptor); // closed member fd 
        }
    }


    close(masterSocket_); // close master fd

    room_->nMembers = 0;
    room_->roomStatus = UNSET;

    for (i = 0; i < MAX_CHARS_IN_ROOM_NAME; i++)
    {
        (room_->roomName)[i] = 0;
    }

    room_->portN = 0;

    room_->nMembers = 0;

    for (i = 0; i < MAX_N_MEMBERS_PER_ROOM; i++)
    {
        (room_->membersSocket)[i] = 0;
    }

    return NULL;
}



#endif // CHAT_ROOM_THREAD_H_