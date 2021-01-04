#ifndef REPLY_ASSIST_H_
#define REPLY_ASSIST_H_

#include "interface.h"

typedef char ReplyType_t;
#define REPLAY_STATUS_ONLY 'S'
#define REPLAY_ROOM_INFORMATION 'R'
#define REPLAY_LIST 'L'


void blankReplay(struct Reply* const reply)
{
    reply->status = FAILURE_UNKNOWN;

    int i;
    for ( i = 0; i < MAX_DATA; i++)
    {
        reply->list_room[i] = '\0';
    }
}


int sizeOfReplay(const char replyType)
{
    struct Reply reply_;
    switch (replyType)
    {
        case REPLAY_STATUS_ONLY:
            return sizeof(reply_.status);
            break;
        case REPLAY_ROOM_INFORMATION:
            return sizeof(reply_.status) + sizeof(reply_.num_member) + sizeof(reply_.port);
            break;
        case REPLAY_LIST:
            return sizeof(reply_);
            break;
        default:
            return 0;
    }

}



#endif // REPLY_ASSIST_H_