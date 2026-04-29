#ifndef ARENA_H
#define ARENA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include <termios.h>
#include <fcntl.h>

#define SHM_KEY 0x00001234
#define MSG_KEY 0x00005678
#define SEM_KEY 0x00009012

#define MAX_USERS 50
#define MAX_BATTLES 10

#define BASE_HP 100
#define BASE_DMG 10

typedef enum {
    REQ_REGISTER =1, REQ_LOGIN, REQ_MATCHMAKE,
    REQ_ATTACK, REQ_BUY_WEAPON, RES_SERVER
} MsgType;

typedef struct {
    char username[50];
    char password[50];
    int gold;
    int xp;
    int level;
    int weapon_dmg; 
    bool is_online;
} PlayerData;

typedef struct {

    bool active;
    char p1_name[50];
    char p2_name [50];
    int p1_hp;
    int p1_max_hp;
    int p2_hp;
    int p2_max_hp;
    char combat_log[5][100];
    int log_count;
    bool is_bot_match;
    char winner[50];

} BattleArena;

typedef struct {

    PlayerData users[MAX_USERS];
    int user_count;
    BattleArena battles[MAX_BATTLES];

} SharedData;

typedef struct {
    long mytype;
    long client_pid;
    char username[50];
    char data1[50];
    int value;
    bool success;
} IpcMessage;

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

void sem_lock(int semid) {
    struct sembuf sb = {0, -1, 0};
    semop(semid, &sb, 1);
}

void sem_unlock(int semid) {
    struct sembuf sb = {0, 1, 0};
    semop(semid, &sb, 1);
}


#endif