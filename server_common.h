#ifndef SERVER_COMMON_H_
#define SERVER_COMMON_H_


#include <stdio.h>  
#include <string.h>   //strlen  
#include <stdlib.h>  
#include <stdbool.h> // bool


#include "interface.h"


#define LISTENING_PORT 2222 // used for listenting to member before they join a group and to send command
#define MAX_N_LISTENING_SOCKETS 50 // just in case there are many people


#define CHAT_ROOMS_STARTING_PORT_NUMBER 3000 // so this has no used outside the program it is used internally to generate chatroom ports
#define MAX_CHARS_IN_ROOM_NAME MAX_DATA
#define MAX_N_ROOMS 30 // number of rooms
#define MAX_N_MEMBERS_PER_ROOM 50 // number of chatting members


enum RoomStatus // used for room status
{
    UNSET,
    INITIATE,
    RUNNING_CHAT_ROOM,
    DELETE_CHAT_ROOM,
    THREAD_CRASHED
};

typedef struct Room { // each chat room is one of these in the memory
    volatile enum RoomStatus roomStatus; // volatile because it is connecting different threads
    char roomName[MAX_CHARS_IN_ROOM_NAME];
    int portN;
    int nMembers;
    int membersSocket[MAX_N_MEMBERS_PER_ROOM];
} Room;


Room roomsDataBase[MAX_N_ROOMS] = {0}; // containing all the rooms as a data base



#endif // SERVER___COMMON_H_