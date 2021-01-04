//Example code: A simple server side code, which echos back the received message. 
//Handle multiple socket connections with select and fd_set on Linux  
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
#include "command_assist.h"
#include "reply_assist.h"
#include "room_assist.h"
#include "chat_room_thread.h"





pthread_t roomsThread[MAX_N_ROOMS]; // these threads are used to run the rooms, once they are needed.


bool processMasterSocketActivities(int masterListeningSocket, int* const listeningSockets);
void processClientSocketActivities(int* const listeningSockets, fd_set* listeningFds);


void processCreate(char* const commandFromCLient, struct Reply* reply);
void processJoin(char* const commandFromCLient, struct Reply* reply, ReplyType_t* replyType);
void processDelete(char* const commandFromCLient, struct Reply* reply);
void processList(char* const commandFromCLient, struct Reply* reply, ReplyType_t* replyType);

     
int main(int argc , char *argv[])   
{   
    int i; // for the iterations in loops
    int masterListeningSocket;
    struct sockaddr_in address;
    int listeningSockets[MAX_N_LISTENING_SOCKETS];  
     
    //initialise all listeningSockets[] to 0 so not checked  
    for (i = 0; i < MAX_N_LISTENING_SOCKETS; i++)   
    {   
        listeningSockets[i] = 0;   
    }

    masterListeningSocket = intiateServerSocket(LISTENING_PORT, &address);
    if (masterListeningSocket == -1)
    {
        exit(EXIT_FAILURE);
    }
 
    //accept the incoming connection   
    printf("Main: Waiting for connections ...\n");   

    // making sure all rooms are unset before we start listening to clients
    unsetAllRooms();

    while(true)   
    {
        //set of socket file descriptors  
        fd_set listeningFds;

        //clear the socket set  
        FD_ZERO(&listeningFds);   
     
        //add master socket to set  
        FD_SET(masterListeningSocket, &listeningFds);   
        int maxFileDescriptor = masterListeningSocket;   
             
        //add child sockets to set  
        for ( i = 0; i < MAX_N_LISTENING_SOCKETS; i++)   
        {   
            //socket descriptor  
            int socketDescriptor = listeningSockets[i];   
                 
            //if valid socket descriptor then add to read list  
            if(socketDescriptor > 0)
            {
                FD_SET( socketDescriptor, &listeningFds);   
                //highest file descriptor number, need it for the select function  
                if(socketDescriptor > maxFileDescriptor)
                    maxFileDescriptor = socketDescriptor;
            }
        }   
     
        //wait for an activity on one of the sockets , timeout is NULL ,  
        //so wait indefinitely  
        int activity = select( maxFileDescriptor + 1, &listeningFds, NULL, NULL, NULL);

        if (activity > 0)
        {
            //If something happened on the master socket,
            //then its an incoming connection
            if (FD_ISSET(masterListeningSocket, &listeningFds))   
            {
                processMasterSocketActivities(masterListeningSocket, listeningSockets);
            }   
                
            processClientSocketActivities(listeningSockets, &listeningFds);
        }
    } // end of while(true)  
         
    return 0;   
}



bool processMasterSocketActivities(int masterListeningSocket, int* const listeningSockets)
{
    // checking for availability
    int availableIndex = MAX_N_LISTENING_SOCKETS;
    int i;
    for (i = 0; i < MAX_N_LISTENING_SOCKETS; i++)   
    {   
        //if position is empty  
        if( listeningSockets[i] == 0 )   
        {   
            availableIndex = i;      
            break;   
        }   
    }

    if (availableIndex >= MAX_N_LISTENING_SOCKETS) // to terminate if there is no availibility
    {
        printf("Main: Listening cappacity is full.   #Clients: %d\n", MAX_N_LISTENING_SOCKETS);
        return false; 
    }

    // there is enough space for new socket
    int newSocket;
    struct sockaddr_in address; 
    int addressLength = sizeof(address);
    if ((newSocket = accept(masterListeningSocket,  (struct sockaddr *)&address, (socklen_t*)&addressLength))<0)   
    {   
        perror("accept");
        return false;  
    }
    
    // new connection IP and port
    printf("Main: new user connected.   ip: %s  |  port: %d\n", 
                inet_ntoa(address.sin_addr), ntohs (address.sin_port));  
        
    //add new socket to array of sockets  
    listeningSockets[availableIndex] = newSocket;
    return true;
}




void processClientSocketActivities(int* const listeningSockets, fd_set* listeningFds)
{
    int i;
    for (i = 0; i < MAX_N_LISTENING_SOCKETS; i++)   
    {   
        int socketDescriptor = listeningSockets[i];
        if ( socketDescriptor != 0)   
        {        
            if (FD_ISSET( socketDescriptor, listeningFds))   
            {   
                //Check if it was for closing , and also read the  
                //incoming message  
                int commandSize;
                char commandFromCLient[MAX_DATA + 1];  // +1 for null terminated string
                if ((commandSize = read( socketDescriptor, commandFromCLient, MAX_DATA)) == 0)   
                {  
                    struct sockaddr_in address; 
                    int addressLength = sizeof(address); 
                    //Somebody disconnected , get his details and print  
                    getpeername(socketDescriptor, (struct sockaddr*)&address, (socklen_t*)&addressLength);   
                    printf("Main: client disconnected.   ip: %s  |  port: %d\n", 
                            inet_ntoa(address.sin_addr), ntohs (address.sin_port));
                        
                    //Close the socket and mark as 0 in list for reuse  
                    close( socketDescriptor );   
                    listeningSockets[i] = 0;   
                }   
                else //Analyze command from client
                {   
                    struct Reply reply;
                    blankReplay(&reply);

                    ReplyType_t replyType = REPLAY_STATUS_ONLY;

                    commandFromCLient[commandSize] = '\0'; // adding the null terminating character
                    toUpperCase(commandFromCLient, 1); // Capitalizing the first charactor
                    
                    switch (commandFromCLient[0]) // checking the first letter
                    {
                        case 'C': // check for C in CREATE
                            processCreate(commandFromCLient, &reply);
                            break;
                        case 'J': // check for J in JOIN
                            processJoin(commandFromCLient, &reply, &replyType);
                            break;
                        case 'D': // check for D in DELETE
                            processDelete(commandFromCLient, &reply);
                            break;
                        case 'L':
                            processList(commandFromCLient, &reply, &replyType);
                            break;
                        default: 
                                reply.status = FAILURE_INVALID;
                                printf("Main: Invalid Command\n");
                            break;
                    } // end of switch

                    // sending reply back
                    int expectedSize = sizeof(replyType);
                    if (send(socketDescriptor, &replyType, expectedSize, 0 )  != -1)
                    {
                        expectedSize = sizeOfReplay(replyType);
                        if (send(socketDescriptor, &reply, expectedSize, 0 )  != -1)
                        {
                            printf("Main: Reply Sent\n"); 
                        }
                        else
                        {
                            printf("Main: Sending Reply Failed\n");
                        }
                        
                    }
                    else
                    {
                        printf("Main: Sending Reply Type Failed\n");  
                    }
                }   
            } 
        }  
    }
}



// this functions assume the first letter is 'C' or 'c' and take care of the rest "CREATE" command
void processCreate(char* const commandFromCLient, struct Reply* reply)
{
    if (doStringsMatchCaseInsensitive(&commandFromCLient[1], &COMMAND_TYPE_CREATE[1])) // Changing REATE in CREATE
    {
        char const* chatRoomName = extractChatRoomName(commandFromCLient, COMMAND_TYPE_CREATE);
        if (chatRoomName != NULL) // making sure name is valid
        {
            int roomIndex = addChatRoom(chatRoomName);
            if (roomIndex >= 0)
            {
                reply->status = SUCCESS;
                roomsDataBase[roomIndex].roomStatus = INITIATE;

                // start the room thread.
                pthread_join(roomsThread[roomIndex], NULL); // making sure the tread is not working, this is just a safety measure to make sure the thread has stopped.
                pthread_create(&roomsThread[roomIndex], NULL, chatRoomRunTimeThread, &roomsDataBase[roomIndex]);

                printf("Main: CREATION Room \"%s\", Thread Started.   i: %d  |  port: %d\n", 
                       roomsDataBase[roomIndex].roomName, roomIndex, roomsDataBase[roomIndex].portN);
            }
            else
            {
                if (roomIndex == -1) // chat room exist
                {
                    reply->status = FAILURE_ALREADY_EXISTS;
                    printf("Main: CREATION stopped (Already Exist)\n");
                }
                else if (roomIndex == -2) // max chatroom capacity
                {
                    reply->status = FAILURE_UNKNOWN;
                    printf("Main: CREATION failed (Full Capacity)\n");
                }
            }
        }
    }
    else
    {
        reply->status = FAILURE_INVALID;
        printf("Main: CREATION failed (Invalid Command)\n");
    }
}



// this functions assume the first letter is 'J' or 'j' and take care of the rest in "JOIN" command
void processJoin(char* const commandFromCLient, struct Reply* reply, ReplyType_t* replyType)
{
    if (doStringsMatchCaseInsensitive(&commandFromCLient[1], &COMMAND_TYPE_JOIN[1])) // Changing OIN in JOIN
    {
        char const* chatRoomName = extractChatRoomName(commandFromCLient, COMMAND_TYPE_JOIN);

        if (chatRoomName != NULL) // making sure name is valid
        {
            int roomIndex = findChatRoomIndex(chatRoomName);
            if ((roomIndex >= 0) && !isRoomGoingToBeDeleted(roomIndex))
            {
                reply->status = SUCCESS;
                reply->num_member = roomsDataBase[roomIndex].nMembers;
                reply->port = roomsDataBase[roomIndex].portN;

                *replyType = REPLAY_ROOM_INFORMATION; 

                printf("Main: JOIN Request for \"%s\".   port: %d  |  #members: %d\n", 
                       roomsDataBase[roomIndex].roomName, roomsDataBase[roomIndex].portN,  roomsDataBase[roomIndex].nMembers);
            }
            else
            {
                reply->status = FAILURE_NOT_EXISTS;
                printf("Main: JOIN Failed (Room Name Does Not Exist)\n");
            }
        }
    }
    else
    {
        reply->status = FAILURE_INVALID;
        printf("Main: JOIN failed (Invalid Command)\n");
    }
}



// this functions assume the first letter is 'D' or 'd' and take care of the rest in "DELETE" command
void processDelete(char* const commandFromCLient, struct Reply* reply)
{
    if (doStringsMatchCaseInsensitive(&commandFromCLient[1], &COMMAND_TYPE_DELETE[1])) // Changing ELETE in DELETE
    {
        char const* chatRoomName = extractChatRoomName(commandFromCLient, COMMAND_TYPE_DELETE);
        if (chatRoomName != NULL) // making sure name is valid
        {
            int roomIndex = findChatRoomIndex(chatRoomName);
            if ((roomIndex >= 0) && !isRoomGoingToBeDeleted(roomIndex))
            {
                roomsDataBase[roomIndex].roomStatus = DELETE_CHAT_ROOM;

                reply->status = SUCCESS;

                printf("Main: DELETE Request for \"%s\".   port: %d  |  #members: %d  |\n", 
                       roomsDataBase[roomIndex].roomName, roomsDataBase[roomIndex].portN,  roomsDataBase[roomIndex].nMembers);
            }
            else
            {
                // chat room does not exist
                reply->status = FAILURE_NOT_EXISTS;
                printf("Main: DELETE Failed (Room Name Does Not Exist)\n");
            }
        }
    }
    else
    {
        reply->status = FAILURE_INVALID;
        printf("Main: DELETE failed (Invalid Command)\n");
    }
}



// this functions assume the first letter is 'L' or 'l' and take care of the rest in "LIST" command
void processList(char* const commandFromCLient, struct Reply* reply, ReplyType_t* replyType)
{
    if (strlen(commandFromCLient) == strlen(COMMAND_TYPE_LIST))
    {
        if (doStringsMatchCaseInsensitive(&commandFromCLient[1], &COMMAND_TYPE_LIST[1])) // Changing OIN in JOIN
        {
            reply->status = SUCCESS;
            *replyType = REPLAY_LIST;
            makeListOfChatRooms(reply);
            printf("Main: LIST Made\n");
        }
        else
        {
            reply->status = FAILURE_INVALID;
            printf("Main: LIST failed (Invalid Command)\n");
        }
    }
    else
    {
        reply->status = FAILURE_INVALID;
        printf("Main: LIST failed (Invalid Command)\n");
    }
    
}