#include "arena.h"

int shmid, msgid, semid;
SharedData *shm;
char my_username[50];
long my_pid;

void clear_screen() {
    system("clear");
}

void print_header() {

    printf("\033[33m");
    printf("  ____        _   _   _         ___   __ \n");
    printf(" | __ )  __ _| |_| |_| | ___   / _ \\ / _|\n");
    printf(" |  _ \\ / _` | __| __| |/ _ \\ | | | | |_ \n");
    printf(" | |_) | (_| | |_| |_| |  __/ | |_| |  _|\n");
    printf(" |____/ \\__,_|\\__|\\__|_|\\___|  \\___/|_|  \n");
    

    printf("\033[36m");
    printf(" | ____| |_ ___ _ __(_) ___  _ __        \n");
    printf(" |  _| | __/ _ \\ '__| |/ _ \\| '_ \\       \n");
    printf(" | |___| ||  __/ |  | | (_) | | | |      \n");
    printf(" |_____|\\__\\___|_|  |_|\\___/|_| |_|      \n");
    printf("\033[0m\n");
}
                                                                                                                                           
                                                                             

void connect_ipc() {
    shmid = shmget(SHM_KEY, sizeof(SharedData), 0666);
    if(shmid < 0) { printf("Orion is not ready. Exiting...\n"); exit(1); }
    shm = (SharedData *)shmat(shmid, NULL, 0);
    msgid = msgget(MSG_KEY, 0666);
    semid = semget(SEM_KEY, 1, 0666);
    my_pid = getpid();
}

void send_request(MsgType type, const char* u, const char* d1, int val, IpcMessage *res) {
    IpcMessage req;
    req.mtype = type;
    req.client_pid = my_pid;
    if(u) strcpy(req.username, u);
    if(d1) strcpy(req.data1, d1);
    req.value = val;
    
    msgsnd(msgid, &req, sizeof(IpcMessage) - sizeof(long), 0);
    msgrcv(msgid, res, sizeof(IpcMessage) - sizeof(long), my_pid, 0);
}

void set_terminal_raw_mode(bool enable) {
    static struct termios oldt, newt;
    if (enable) {
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
    } else {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) & ~O_NONBLOCK);
    }
}

void print_hp_bar(int hp, int max_hp, const char* color_code) {
    int max_blocks = 20;
    int current_blocks = (max_hp > 0) ? (hp * max_blocks) / max_hp : 0;
    if (current_blocks < 0) current_blocks = 0;

    printf("[");
    printf("%s", color_code);
    for (int i = 0; i < max_blocks; i++) {
        if (i < current_blocks) printf("█");
        else printf(" ");
    }
    printf("\033[0m] %d/%d\n", hp, max_hp);
}

void draw_arena(char* enemy_name, int enemy_lvl, int enemy_hp, int enemy_max_hp, 
                char* my_name, int my_lvl, int my_hp, int my_max_hp, 
                char* my_weapon, char combat_log[5][100], double atk_cd, double ult_cd) {
    clear_screen();
    printf("\033[31m┌──────────────────── ARENA ────────────────────┐\033[0m\n\n");
    printf("\033[36m%-15s\033[0m Lvl %d\n", enemy_name, enemy_lvl);
    print_hp_bar(enemy_hp, enemy_max_hp, "\033[31m");
    printf("\n                 \033[35mVS\033[0m\n\n");
    printf("\033[36m%-15s\033[0m Lvl %d | \033[33mWeapon: %s\033[0m\n", my_name, my_lvl, my_weapon);
    print_hp_bar(my_hp, my_max_hp, "\033[32m");
    printf("\n\033[33mCombat Log:\033[0m\n");
    for(int i = 0; i < 5; i++) {
        if(strlen(combat_log[i]) > 0) printf("> %s\n", combat_log[i]);
        else printf(">\n");
    }
    printf("\n\033[36mCD:\033[0m Atk(%.1fs) | Ult(%.1fs)\n", atk_cd, ult_cd);
    printf("\033[31m└───────────────────────────────────────────────┘\033[0m\n");
}

void battle_loop(int arena_id, PlayerData *me) {
    set_terminal_raw_mode(true);
    
    sem_lock(semid);
    bool is_p1 = (strcmp(shm->battles[arena_id].p1_name, my_username) == 0);
    sem_unlock(semid);

    time_t last_atk_time = 0;
    time_t last_ult_time = 0;

    char enemy_name_final[50];
    int earned_xp = 0;
    bool is_win = false;

    while (1) {
        sem_lock(semid);
        BattleArena *b = &shm->battles[arena_id];
        
        int my_hp = is_p1 ? b->p1_hp : b->p2_hp;
        int enemy_hp = is_p1 ? b->p2_hp : b->p1_hp;
        strcpy(enemy_name_final, is_p1 ? b->p2_name : b->p1_name);
        
        if (my_hp <= 0 || enemy_hp <= 0) {
            sem_unlock(semid);
            set_terminal_raw_mode(false);
            clear_screen();
            
            if (my_hp <= 0) {
                printf("\033[31m--- DEFEAT ---\033[0m\n\n");
                earned_xp = 15;
                me->xp += earned_xp;
                me->gold += 30;
            } else {
                printf("\033[32m--- VICTORY ---\033[0m\n\n");
                earned_xp = 50;
                me->xp += earned_xp;
                me->gold += 120;
                is_win = true;
            }
            
            me->level = 1 + (me->xp / 100);
            
            printf("Battle ended. Press [ENTER] to continue...\n");
            
            int c; while ((c = getchar()) != '\n' && c != EOF);
            
            sem_lock(semid);
            b->active = false;
            
            for(int i=0; i<shm->user_count; i++) {
                if(strcmp(shm->users[i].username, my_username) == 0) {
                    shm->users[i] = *me; 
                    break;
                }
            }
            sem_unlock(semid);
            
            char filename[100];
            sprintf(filename, "%s_history.txt", my_username);
            FILE *hf = fopen(filename, "a");
            if(hf) {
                char time_str[20];
                time_t now = time(NULL);
                strftime(time_str, sizeof(time_str), "%H:%M", localtime(&now));
                fprintf(hf, "%s|%s|%s|+%d XP\n", time_str, enemy_name_final, is_win ? "WIN" : "LOSS", earned_xp);
                fclose(hf);
            }

            IpcMessage res;
            send_request(REQ_LOGIN, me->username, me->password, 0, &res);
            break;
        }

        char enemy_name[50], my_name[50];
        int my_max_hp = is_p1 ? b->p1_max_hp : b->p2_max_hp;
        int enemy_max_hp = is_p1 ? b->p2_max_hp : b->p1_max_hp;
        strcpy(my_name, is_p1 ? b->p1_name : b->p2_name);
        strcpy(enemy_name, is_p1 ? b->p2_name : b->p1_name);
        
        int enemy_lvl = 1;
        char enemy_weapon[50] = "None";
        if (!b->is_bot_match) {
            for(int i=0; i<shm->user_count; i++) {
                if(strcmp(shm->users[i].username, enemy_name) == 0) {
                    enemy_lvl = shm->users[i].level;
                    if(shm->users[i].weapon_dmg > 0) strcpy(enemy_weapon, "Equipped");
                    break;
                }
            }
        }
        
        char my_weapon[50] = "None";
        if (me->weapon_dmg > 0) strcpy(my_weapon, "Equipped");

        time_t now = time(NULL);
        double atk_cd = (now - last_atk_time >= 1) ? 0.0 : 1.0 - difftime(now, last_atk_time);
        double ult_cd = (now - last_ult_time >= 1) ? 0.0 : 1.0 - difftime(now, last_ult_time);

        draw_arena(enemy_name, enemy_lvl, enemy_hp, enemy_max_hp,
                   my_name, me->level, my_hp, my_max_hp,
                   my_weapon, b->combat_log, atk_cd, ult_cd);

        sem_unlock(semid);

        char key = getchar();
        if (key == 'a' || key == 'A') {
            if (now - last_atk_time >= 1) { 
                last_atk_time = now;
                int dmg = BASE_DMG + (me->xp / 50) + me->weapon_dmg;
                
                sem_lock(semid);
                if (is_p1) shm->battles[arena_id].p2_hp -= dmg;
                else shm->battles[arena_id].p1_hp -= dmg;
                
                char log_msg[100];
                sprintf(log_msg, "You hit for %d damage!", dmg);
                for(int i=4; i>0; i--) strcpy(shm->battles[arena_id].combat_log[i], shm->battles[arena_id].combat_log[i-1]);
                strcpy(shm->battles[arena_id].combat_log[0], log_msg);
                sem_unlock(semid);
            }
        } 
        else if (key == 'u' || key == 'U') {
             if (me->weapon_dmg > 0 && now - last_ult_time >= 1) {
                last_ult_time = now;
                int dmg = (BASE_DMG + (me->xp / 50) + me->weapon_dmg) * 3;
                
                sem_lock(semid);
                if (is_p1) shm->battles[arena_id].p2_hp -= dmg;
                else shm->battles[arena_id].p1_hp -= dmg;
                
                char log_msg[100];
                sprintf(log_msg, "ULTIMATE! You hit for %d damage!", dmg);
                for(int i=4; i>0; i--) strcpy(shm->battles[arena_id].combat_log[i], shm->battles[arena_id].combat_log[i-1]);
                strcpy(shm->battles[arena_id].combat_log[0], log_msg);
                sem_unlock(semid);
             }
        }
        
        if (b->is_bot_match) {
            static time_t last_bot_atk = 0;
            if (now - last_bot_atk >= 2) { 
                last_bot_atk = now;
                int bot_dmg = 8;
                sem_lock(semid);
                if (shm->battles[arena_id].p1_hp > 0 && shm->battles[arena_id].p2_hp > 0) {
                     shm->battles[arena_id].p1_hp -= bot_dmg;
                     char log_msg[100];
                     sprintf(log_msg, "Wild Beast hit you for %d damage!", bot_dmg);
                     for(int i=4; i>0; i--) strcpy(shm->battles[arena_id].combat_log[i], shm->battles[arena_id].combat_log[i-1]);
                     strcpy(shm->battles[arena_id].combat_log[0], log_msg);
                }
                sem_unlock(semid);
            }
        }
        usleep(50000); 
    }
}

void armory_menu(PlayerData *me) {
    while(1) {
        clear_screen();
        printf("\033[33m═══ ARMORY ═══\033[0m\n");
        printf("Gold: %d\n\n", me->gold);
        printf("1. Wood Sword   |  100 G | +5   Dmg\n");
        printf("2. Iron Sword   |  300 G | +15  Dmg\n");
        printf("3. Steel Axe    |  600 G | +30  Dmg\n");
        printf("4. Demon Blade  | 1500 G | +60  Dmg\n");
        printf("5. God Slayer   | 5000 G | +150 Dmg\n");
        printf("0. Back | Choice: ");
        
        int c; 
        if(scanf("%d", &c) != 1) { getchar(); continue; }
        if(c == 0) break;

        int price = 0, dmg = 0;
        if(c==1){ price=100; dmg=5; }
        else if(c==2){ price=300; dmg=15; }
        else if(c==3){ price=600; dmg=30; }
        else if(c==4){ price=1500; dmg=60; }
        else if(c==5){ price=5000; dmg=150; }

        if (price > 0 && me->gold >= price) {
            me->gold -= price;
            if (dmg > me->weapon_dmg) me->weapon_dmg = dmg;
            
            sem_lock(semid);
            for(int i=0; i<shm->user_count; i++){
                if(strcmp(shm->users[i].username, my_username)==0){
                    shm->users[i] = *me; break;
                }
            }
            sem_unlock(semid);
            
            printf("\033[32mWeapon purchased & equipped!\033[0m\n");
        } else if (price > 0) {
            printf("\033[31mNot enough gold!\033[0m\n");
        }
        sleep(1);
    }
}

void history_menu() {
    clear_screen();
    printf("\033[35m┌──────────────── MATCH HISTORY ────────────────┐\033[0m\n");
    printf("\033[35m│\033[0m %-10s │ %-15s │ %-4s │ %-6s \033[35m │\033[0m\n", "Time", "Opponent", "Res", "XP");
    printf("\033[35m├───────────────────────────────────────────────┤\033[0m\n");
    
    char filename[100];
    sprintf(filename, "%s_history.txt", my_username);
    FILE *hf = fopen(filename, "r");
    if (hf) {
        char line[256];
        while (fgets(line, sizeof(line), hf)) {
            line[strcspn(line, "\n")] = 0;
            char *time_str = strtok(line, "|");
            char *opp = strtok(NULL, "|");
            char *res = strtok(NULL, "|");
            char *xp = strtok(NULL, "|");
            if(time_str && opp && res && xp) {
                char res_color[20];
                if(strcmp(res, "WIN")==0) strcpy(res_color, "\033[32mWIN\033[0m ");
                else strcpy(res_color, "\033[31mLOSS\033[0m");
                
                printf("\033[35m│\033[0m %-10s │ %-15s │ %s │ %-6s \033[35m │\033[0m\n", time_str, opp, res_color, xp);
            }
        }
        fclose(hf);
    } else {
        printf("\033[35m│\033[0m %-43s \033[35m │\033[0m\n", "No match history found.");
    }
    printf("\033[35m└───────────────────────────────────────────────┘\033[0m\n\n");
    printf("Press [ENTER] to return...");
    int c; while ((c = getchar()) != '\n' && c != EOF);
    getchar();
} 

void print_profile_box(PlayerData *me) {
    printf("\033[34m┌──────────────── PROFILE ────────────────┐\033[0m\n");
    printf("\033[34m│\033[0m \033[36mName\033[0m : %-15s Lvl : %-8d \033[34m  │\033[0m\n", me->username, me->level);
    printf("\033[34m│\033[0m \033[33mGold\033[0m : %-15d XP  : %-8d \033[34m  │\033[0m\n", me->gold, me->xp);
    printf("\033[34m└─────────────────────────────────────────┘\033[0m\n\n");
}

void main_menu() {
    while(1) {
        sem_lock(semid);
        PlayerData me;
        for(int i=0; i<shm->user_count; i++) {
            if(strcmp(shm->users[i].username, my_username) == 0) {
                me = shm->users[i]; break;
            }
        }
        sem_unlock(semid);

        clear_screen();
        print_header();
        print_profile_box(&me);

        printf("\033[34m┌───────────────────────┐\033[0m\n");
        printf("\033[34m│\033[0m 1. Battle             \033[34m│\033[0m\n");
        printf("\033[34m│\033[0m 2. Armory             \033[34m│\033[0m\n");
        printf("\033[34m│\033[0m 3. History            \033[34m│\033[0m\n");
        printf("\033[34m│\033[0m 4. Logout             \033[34m│\033[0m\n");
        printf("\033[34m└───────────────────────┘\033[0m\n");
        printf("\n> Choice: ");
        
        int c; 
        if (scanf("%d", &c) != 1) { getchar(); continue; }


        if (c == 1) {
            clear_screen();
            print_header();
            
            IpcMessage res;
            send_request(REQ_MATCHMAKE, my_username, "", me.xp, &res);
            
            if (!res.success) {
                printf("Server full! Cannot create battle.\n"); sleep(2); continue;
            }
            
            int arena_id = res.value;
            bool match_found = false;
            
            for (int wait = 0; wait < 35; wait++) {
                printf("\r\033[31mSearching for an opponent... [%d s] \033[0m", 35 - wait);
                fflush(stdout);
                
                sem_lock(semid);
                if (strlen(shm->battles[arena_id].p2_name) > 0) {
                    match_found = true;
                    sem_unlock(semid);
                    break;
                }
                sem_unlock(semid);
                sleep(1);
            }

            if (!match_found) {
                sem_lock(semid);
                strcpy(shm->battles[arena_id].p2_name, "Wild Beast");
                shm->battles[arena_id].p2_max_hp = 120;
                shm->battles[arena_id].p2_hp = 120;
                shm->battles[arena_id].is_bot_match = true;
                sem_unlock(semid);
            }
            
            printf("\n\n\033[32mMatch Found! Entering Arena...\033[0m\n");
            sleep(1);
            
            battle_loop(arena_id, &me);
            
        }

        else if (c == 2) {
            armory_menu(&me);
        }
        else if (c == 3) {
            history_menu();
        }
        else if (c == 4) {
            sem_lock(semid);
            for(int i=0; i<shm->user_count; i++) {
                if(strcmp(shm->users[i].username, my_username) == 0) {
                    shm->users[i].is_online = false; break;
                }
            }
            sem_unlock(semid);
            break; 
        }
    }
}

void menu_auth() {
    while(1) {
        clear_screen();
        print_header();
        printf(" 1. Register\n");
        printf(" 2. Login\n");
        printf(" 3. Exit\n");
        printf(" Choice: ");
        
        int c; 
        if (scanf("%d", &c) != 1) { getchar(); continue; }

        if(c == 3) exit(0);
        
        char u[50], p[50];
        IpcMessage res;

        if (c == 1) {
            printf("\n\033[36mCREATE ACCOUNT\033[0m\n"); 
            printf("Username: "); scanf("%s", u);
            printf("Password: "); scanf("%s", p);

            send_request(REQ_REGISTER, u, p, 0, &res);
            if(res.success) {
                printf("\033[32m Account created!\033[0m\n"); 
                sleep(2);
            } else {
                printf("\033[31m Register failed. Username exists.\033[0m\n"); 
                sleep(2);
            }
        } 
        else if (c == 2) {
            printf("\n\033[36mLOGIN\033[0m\n");
            printf("Username: "); scanf("%s", u);
            printf("Password: "); scanf("%s", p);

            send_request(REQ_LOGIN, u, p, 0, &res);
            if(res.success) {
                printf("\033[32m Welcome!\033[0m\n");
                strcpy(my_username, u);
                sleep(1);
                return; 
            } else {
                printf("\033[31m Login failed. Wrong credentials or account online.\033[0m\n");
                sleep(2);
            }
        }
    }
}

int main() {
    connect_ipc();
    printf("\033[31mOrion are you there?\033[0m\n");
    sleep(1);
    
    while(1) {
        menu_auth();
        main_menu();
    }
    return 0;
}