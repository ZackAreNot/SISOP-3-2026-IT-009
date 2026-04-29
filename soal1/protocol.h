#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdio.h>
#include<stdlib.h>
#include<string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <signal.h>
#include <time.h>
#include <stdbool.h>

#define PORT 8080
#define MAX_CLIENTS 30
#define BUFFER_SIZE 1024
#define ADMIN_NAME "The Knights"
#define ADMIN_PASS "protocol7"

typedef enum {
    MSG_LOGIN, MSG_LOGIN_OK, MSG_LOGIN_FAIL, 
    MSG_CHAT, MSG_ADMIN_AUTH, MSG_RPC_GET_USERS,
    MSG_RPC_GETUPTIME, MSG_RPC_SHUTDOWN, MSG_EXIT, MSG_SERVER_MSG
} MsgType;

typedef struct {
    MsgType type;
    char sender[50];
    char data[BUFFER_SIZE];
} Packet;

#endif
