#ifndef COMMAND_ASSIST_H_
#define COMMAND_ASSIST_H_

#include <stdio.h>  
#include <string.h>   //strlen  
#include <stdlib.h>  
#include <stdbool.h> // bool

#include "server_common.h"



const char* COMMAND_TYPE_CREATE = "CREATE ";
const char* COMMAND_TYPE_JOIN = "JOIN ";
const char* COMMAND_TYPE_DELETE = "DELETE ";
const char* COMMAND_TYPE_LIST = "LIST";


// convert to upper case but only for nCharacters specified
void toUpperCase(char * const str, const int nCharacters)
{
	int i;
    for (i = 0; i < nCharacters; i++)
        str[i] = toupper((unsigned char)str[i]);
}



// compare case insensetively, a given string with known reference in caprital letters for length of referenceSize are compared
bool doStringsMatchCaseInsensitive(char* const str1, char const* const referenceInCapital)
{
    int str1Size = strlen(str1);
    int referenceSize = strlen(referenceInCapital);

    if (str1Size >= referenceSize)
    {
        toUpperCase(str1, referenceSize); // Changing to capital
        if (strncmp(str1, referenceInCapital, referenceSize) == 0) // checking if command start with DELETE
        {
            return true;
        }
    }
    return false;
}



// extract chat room name from command message replied by client, make sure command does not contain the last enter and instead is null terminated
char const* extractChatRoomName(char const* const command, char const* const commandType)
{
    int commandSize = strlen(command);
    // printf("command type size: %d\n", commandSize);
    
    int commandTypeSize = strlen(commandType);
    // printf("commandTypeSize: %d\n", commandTypeSize);

    int nameSize = commandSize - commandTypeSize;
    // printf("Name Size: %d\n", nameSize);

    if (nameSize > 0)
    {
        return &command[commandTypeSize];
    }
    else
    {
        return NULL;
    }
}



#endif // COMMAND_ASSIST_H_