#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "interface.h"


#include <arpa/inet.h>    //inet_pton 
#include <errno.h>
#include "command_assist.h"
#include "reply_assist.h"



/*
 * TODO: IMPLEMENT BELOW THREE FUNCTIONS
 */
int connect_to(const char *host, const int port);
struct Reply process_command(const int sockfd, char* command);
void process_chatmode(const char* host, const int port);

int main(int argc, char** argv) 
{
	if (argc != 3) {
		fprintf(stderr,
				"usage: enter host address and port number\n");
		exit(1);
	}

    display_title();
    
	while (1) {
	
		int sockfd = connect_to(argv[1], atoi(argv[2]));
    
		char command[MAX_DATA];
        get_command(command, MAX_DATA);

		struct Reply reply = process_command(sockfd, command);
		display_reply(command, reply);
		
		touppercase(command, strlen(command) - 1);
		if ((strncmp(command, "JOIN", 4 )  == 0) && (reply.status == SUCCESS)) {
			printf("Now you are in the chatmode\n");
			process_chatmode(argv[1], reply.port);
		}
	
		close(sockfd);
    }

    return 0;
}



/*
 * Connect to the server using given host and port information
 *
 * @parameter host    host address given by command line argument
 * @parameter port    port given by command line argument
 * 
 * @return socket fildescriptor
 */
int connect_to(const char *host, const int port)
{
	// ------------------------------------------------------------
	// GUIDE :
	// In this function, you are suppose to connect to the server.
	// After connection is established, you are ready to send or
	// receive the message to/from the server.
	// 
	// Finally, you should return the socket fildescriptor
	// so that other functions such as "process_command" can use it
	// ------------------------------------------------------------

	int sockfd;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
		// printf("\n Socket creation error \n"); 
        return -1; 
    }

	struct sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);

	// Convert IPv4 and IPv6 addresses from text to binary form 
    if (inet_pton(AF_INET, host, &serverAddress.sin_addr) <= 0)  
    { 
        // printf("\nInvalid address/ Address not supported \n"); 
        return -1; 
    } 


	if (connect(sockfd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) 
    { 
        // printf("\nConnection Failed \n"); 
        return -1; 
    } 

	return sockfd;
}



/* 
 * Send an input command to the server and return the result
 *
 * @parameter sockfd   socket file descriptor to commnunicate
 *                     with the server
 * @parameter command  command will be sent to the server
 *
 * @return    Reply    
 */
struct Reply process_command(const int sockfd, char* command)
{
	// ------------------------------------------------------------
	// GUIDE 1:
	// In this function, you are supposed to parse a given command
	// and create your own message in order to communicate with
	// the server. Surely, you can use the input command without
	// any changes if your server understand it. The given command
    // will be one of the followings:
	//
	// CREATE <name>
	// DELETE <name>
	// JOIN <name>
    // LIST
	//
	// -  "<name>" is a chatroom name that you want to create, delete,
	// or join.
	// 
	// - CREATE/DELETE/JOIN and "<name>" are separated by one space.
	// ------------------------------------------------------------


	// ------------------------------------------------------------
	// GUIDE 2:
	// After you create the message, you need to send it to the
	// server and receive a result from the server.
	// ------------------------------------------------------------


	// ------------------------------------------------------------
	// GUIDE 3:
	// Then, you should create a variable of Reply structure
	// provided by the interface and initialize it according to
	// the result.
	//
	// For example, if a given command is "JOIN room1"
	// and the server successfully created the chatroom,
	// the server will reply a message including information about
	// success/failure, the number of members and port number.
	// By using this information, you should set the Reply variable.
	// the variable will be set as following:
	//
	// Reply reply;
	// reply.status = SUCCESS;
	// reply.num_member = number;
	// reply.port = port;
	// 
	// "number" and "port" variables are just an integer variable
	// and can be initialized using the message fomr the server.
	//
	// For another example, if a given command is "CREATE room1"
	// and the server failed to create the chatroom becuase it
	// already exists, the Reply varible will be set as following:
	//
	// Reply reply;
	// reply.status = FAILURE_ALREADY_EXISTS;
    // 
    // For the "LIST" command,
    // You are suppose to copy the list of chatroom to the list_room
    // variable. Each room name should be seperated by comma ','.
    // For example, if given command is "LIST", the Reply variable
    // will be set as following.
    //
    // Reply reply;
    // reply.status = SUCCESS;
    // strcpy(reply.list_room, list);
    // 
    // "list" is a string that contains a list of chat rooms such 
    // as "r1,r2,r3,"
	// ------------------------------------------------------------
	struct Reply reply;
	blankReplay(&reply);

	int expectedSize = strlen(command);
	if (send(sockfd, command, expectedSize, 0 )  == -1)
	{
		reply.status = FAILURE_UNKNOWN;
		return reply;
	}

	ReplyType_t replyType; // getting the reply type from server
	expectedSize = sizeof(ReplyType_t);
	if (read(sockfd, &replyType, expectedSize) != expectedSize)
	{
		reply.status = FAILURE_UNKNOWN;
		return reply;
	}

	expectedSize = sizeOfReplay(replyType); // getting the size based on reply size
	if (read(sockfd, &reply, expectedSize) != expectedSize)
	{
		reply.status = FAILURE_UNKNOWN;
		return reply;
	}

	if (replyType == REPLAY_ROOM_INFORMATION) // if the user wanted to join the chat room, he should close this socket. The new chat socket will be open later in process_chatmode
	{
		close(sockfd);
	}

	return reply;
}



/* 
 * Get into the chat mode
 * 
 * @parameter host     host address
 * @parameter port     port
 */
void process_chatmode(const char* host, const int port)
{
	// ------------------------------------------------------------
	// GUIDE 1:
	// In order to join the chatroom, you are supposed to connect
	// to the server using host and port.
	// You may re-use the function "connect_to".
	// ------------------------------------------------------------

	// ------------------------------------------------------------
	// GUIDE 2:
	// Once the client have been connected to the server, we need
	// to get a message from the user and send it to server.
	// At the same time, the client should wait for a message from
	// the server.
	// ------------------------------------------------------------
	
    // ------------------------------------------------------------
    // IMPORTANT NOTICE:
    // 1. To get a message from a user, you should use a function
    // "void get_message(char*, int);" in the interface.h file
    // 
    // 2. To print the messages from other members, you should use
    // the function "void display_message(char*)" in the interface.h
    //
    // 3. Once a user entered to one of chatrooms, there is no way
    //    to command mode where the user  enter other commands
    //    such as CREATE,DELETE,LIST.
    //    Don't have to worry about this situation, and you can 
    //    terminate the client program by pressing CTRL-C (SIGINT)
	// ------------------------------------------------------------
	int sockfd = connect_to(host, port);

	bool chatFlag = true;

	while(chatFlag)
	{
		//set of socket file descriptors for stdin and from sockfd
        fd_set memberFds;

        //clear the socket set  
        FD_ZERO(&memberFds);   
     
        //add master socket to set 
		FD_SET(sockfd, &memberFds); 
        FD_SET(STDIN_FILENO, &memberFds);
		

		int maxFileDescriptor = sockfd;
		if (maxFileDescriptor < STDIN_FILENO)
		{
			maxFileDescriptor = STDIN_FILENO;
		}


		int activity = select(maxFileDescriptor + 1, &memberFds, NULL, NULL, NULL);
		
		if (activity > 0) // check if there has been an activity
		{
			if (FD_ISSET(STDIN_FILENO, &memberFds)) // check stdin activity
			{
				// printf("STDIN_FILENO Select\n");
				char message[MAX_DATA] = {0};
				get_message(message, MAX_DATA); // get chat message from user
				
				int mesageSize = strlen(message); // find the size of user's message
				message[mesageSize] = '\n'; // get_message changes the enter at the end to null pointer. This results in select not detecting it.
				mesageSize++; // to count for \n

				if (send(sockfd, message, mesageSize, 0) != -1) // send user message to the server
				{
					// successful
				}
			}

			if (FD_ISSET(sockfd, &memberFds)) // check socket activity
			{	
				char message[MAX_DATA + 1] = {0}; // +1 for null terminator
				int messageSize = read(sockfd, message, MAX_DATA);
				if (messageSize != 0)
				{
					display_message(message);
				}
				else
				{
					// display_message("server disconnected.\n");
					chatFlag = false;
					display_title();
				}
			}
		}
	}
}

