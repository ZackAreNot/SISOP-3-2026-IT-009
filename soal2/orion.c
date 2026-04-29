#include "arena.h"

int shmid, msgid, semid;
SharedData *shm;

void init_ipc() {

    shmid = shmget(SHM_KEY, sizeof(SharedData), IPC_CREAT | 0666);
    shm = (SharedData *)shmat(shmid, NULL, 0);

    msgid = msgget(MSG_KEY, IPC_CREAT | 0666);

    semid = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    union semun su;
    su.val = 1;
    semctl(semid, 0, SETVAL, su);
}

void load_db() {
    sem_lock(semid);
    FILE *f = fopen("eterion.db", "rb");
    if (f) {
        fread(&shm->user_count, sizeof(int), 1, f);
        fread(shm->users, sizeof(PlayerData), shm->user_count, f);
        fclose(f);
    } else {
        shm->user_count = 0;
    }

    for(int i=0; i < shm->user_count; i++) shm->users[i].is_online = false;
    for(int i=0; i < MAX_BATTLES; i++) shm->battles[i].active = false;
    sem_unlock(semid);
}

void save_db() {
    FILE *f = fopen("eterion.db", "wb");
    if (f) {
        fwrite(&shm->user_count, sizeof(int), 1, f);
        fwrite(shm->users, sizeof(PlayerData), shm->user_count, f);
        fclose(f);
    }
}

int main() {
    printf("Orion is ready (PID: %d)\n", getpid());
    
    init_ipc();
    load_db();

    IpcMessage req;
    
    while(1) {
        if (msgrcv(msgid, &req, sizeof(IpcMessage) - sizeof(long), 0, 0) > 0) {
            IpcMessage res = { .mtype = req.client_pid }; // Kirim balasan ke PID spesifik

            sem_lock(semid);
            if (req.mtype == REQ_REGISTER) {
                bool exists = false;
                for (int i=0; i < shm->user_count; i++) {
                    if (strcmp(shm->users[i].username, req.username) == 0) { exists = true; break; }
                }
                if (exists || shm->user_count >= MAX_USERS) {
                    res.success = false;
                } else {
                    int idx = shm->user_count++;
                    strcpy(shm->users[idx].username, req.username);
                    strcpy(shm->users[idx].password, req.data1);
                    shm->users[idx].gold = 150;
                    shm->users[idx].xp = 0;
                    shm->users[idx].level = 1;
                    shm->users[idx].weapon_dmg = 0;
                    shm->users[idx].is_online = false;
                    save_db();
                    res.success = true;
                }
                msgsnd(msgid, &res, sizeof(IpcMessage) - sizeof(long), 0);
            } 
            else if (req.mtype == REQ_LOGIN) {
                res.success = false;
                for (int i=0; i < shm->user_count; i++) {
                    if (strcmp(shm->users[i].username, req.username) == 0 && 
                        strcmp(shm->users[i].password, req.data1) == 0) {
                        if (!shm->users[i].is_online) {
                            shm->users[i].is_online = true;
                            res.success = true;
                            res.value = shm->users[i].xp; 
                        }
                        break;
                    }
                }
                msgsnd(msgid, &res, sizeof(IpcMessage) - sizeof(long), 0);
            }

            sem_unlock(semid);

        }
    }
    return 0;
}