#ifndef COMMON_H
#define COMMON_H

#define MSG_NAME "READY_MSG"

struct ready_payload {
    int port_id;
    int status;
};

#endif
