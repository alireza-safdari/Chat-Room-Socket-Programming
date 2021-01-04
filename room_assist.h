#ifndef ROOM_ASSIST_H_
#define ROOM_ASSIST_H_

#include <stdio.h>  
#include <string.h>   //strlen  
#include <stdlib.h>  
#include <stdbool.h> // bool

#include "server_common.h"
#include "interface.h"



int addChatRoom(char const* const chatRoomName);
int findChatRoomIndex(char const* const chatRoomName);
int getAnUnsetRoomIndex(void);
bool isRoomGoingToBeDeleted(int roomIndex);
bool isRoomSet(int roomIndex);
void makeListOfChatRooms(struct Reply *const reply);
void setRoom(int roomIndex, char const* const chatRoomName);
void unsetAllRooms(void);
void unsetRoom(struct Room* const room_);



// unset a room
void unsetRoom(struct Room* const room_)
{
    // blanking the room name area
    int i;
    for (i = 0; i < MAX_CHARS_IN_ROOM_NAME; i++)
    {
        (room_->roomName)[i] = 0;
    }

    room_->portN = 0;

    room_->nMembers = 0;

    // making member socket fd all equal to 0, 0 is used to know they are not set
    for (i = 0; i < MAX_N_MEMBERS_PER_ROOM; i++)
    {
        room_->membersSocket[i] = 0;
    }

    room_->roomStatus = UNSET; // we change this last so that from outside threads no race condition occure
}




// set a room, make sure the chatRoomName is null terminated
void setRoom(int roomIndex, char const* const chatRoomName)
{
    // setting state to be initiated in thread
    roomsDataBase[roomIndex].roomStatus = INITIATE;

    // setting name
    int nameSize = strlen(chatRoomName);
    memcpy (roomsDataBase[roomIndex].roomName, chatRoomName, nameSize);
    roomsDataBase[roomIndex].roomName[nameSize] = '\0'; // this is added to make the char room name nul terminated string

    // setting port number
    roomsDataBase[roomIndex].portN = CHAT_ROOMS_STARTING_PORT_NUMBER + roomIndex;;
}



// unset all room
void unsetAllRooms(void)
{
    int i;
    for (i = 0; i < MAX_N_ROOMS; i++)
    {
        unsetRoom(&roomsDataBase[i]);
    }
}



// return true if the roomIndex is Unset
bool isRoomSet(int roomIndex)
{
    int roomStatus_ = roomsDataBase[roomIndex].roomStatus;
    if (roomStatus_ != UNSET)
    {
        return true;
    }

    return false;
}




bool isRoomGoingToBeDeleted(int roomIndex)
{
    int roomStatus_ = roomsDataBase[roomIndex].roomStatus;
    if (roomStatus_ == DELETE_CHAT_ROOM)
    {
        return true;
    }

    return false;
}



// return an index for an Unset room, if no room is available it will return -1
int getAnUnsetRoomIndex(void)
{
    int i;
    for (i = 0; i < MAX_N_ROOMS; i++)
    {
        if (isRoomSet(i) == false)
        {
            return i;
        }
    }
    return -1;
}



// Add a chat room based on its name, return room index or -1 if the room alrady exist or -2 if the room limit has reacched
int addChatRoom(char const* const chatRoomName)
{
    if (findChatRoomIndex(chatRoomName) == -1)
    {
        int roomIndex = getAnUnsetRoomIndex();
        if (roomIndex >= 0)
        {
            printf("setting room, index: %d   | \n", roomIndex);
            setRoom(roomIndex, chatRoomName);
            return roomIndex;
            // create a threat and create the master treat
        }
        else
        {
            return -2; // no room is avialable
        }
        
    }
    else
    {
        return -1; // the room already exist
    } 
}



// find a chat room based on its name, return room index or -1 if it cannot find a room with the given name, Make sure chatRoomName is null terminated
int findChatRoomIndex(char const* const chatRoomName)
{
    int nameSize = strlen(chatRoomName);
    
    int index = -1;
    int i;
    for (i = 0; i < MAX_N_ROOMS; i++)
    {
        if (isRoomSet(i))
        {
            if (nameSize == strlen(roomsDataBase[i].roomName))
            {
                if (strncmp(roomsDataBase[i].roomName, chatRoomName, nameSize) == 0)
                {
                    index = i;
                    break;
                }
            }
        }
    }

    return index;
}



void makeListOfChatRooms(struct Reply *const reply)
{
    int indexForWritingInReplay = 0;
    int i;
    for (i = MAX_N_ROOMS; i >= 0; i--)
    {
       if ( isRoomSet(i) && !isRoomGoingToBeDeleted(i)) // making sure the room is set and not going to be deleted 
       {
           int requiredSize = strlen(roomsDataBase[i].roomName) + 1; // + 1 for comma between names
           if (indexForWritingInReplay + requiredSize < MAX_DATA)
           {
               // coppying the name
               memcpy( &((reply->list_room)[indexForWritingInReplay]), 
                       roomsDataBase[i].roomName, 
                       requiredSize - 1); // - 1 for comma between names

               indexForWritingInReplay += requiredSize - 1; // - 1 for comma between names

                // adding the comma
               (reply->list_room)[indexForWritingInReplay] = ',';
               indexForWritingInReplay++;
           }
           else
           {
               // coppying part of the name that fits
               int emptySpace = MAX_DATA - indexForWritingInReplay;
               memcpy( &((reply->list_room)[indexForWritingInReplay]),
                       roomsDataBase[i].roomName,
                       emptySpace);
               break;
           }
           
       }
    }

    if (indexForWritingInReplay == 0) // no room existed
    {
        char emptyList[]= "empty"; 
        memcpy( (reply->list_room), 
                       emptyList,
                       strlen(emptyList));
    }
}



#endif // ROOM_ASSIST_H_