#include "protocol.h"

int sock = 0;
char username[50];
bool is_admin = false;

void handle_exit(int sig){

    Packet p = {MSG_EXIT, "", ""};
    strcpy(p.sender, username);
    send(sock, &p, sizeof(Packet), 0);
    printf("\n[System] Disconnecting from The Wired...\n");
    close(sock);
    exit(0);

}

void *receive_thread(void *arg){

    Packet p;

    while (recv(sock, &p, sizeof(Packet), 0) > 0){
        if (p.type == MSG_CHAT){
            printf("\r\033[K[%s]: %s\n> ", p.sender, p.data);
            fflush(stdout);
        }

        else if (p.type == MSG_SERVER_MSG){

            if(is_admin){
                printf("\r\033[K[System] %s\n ", p.data);
            } else {
                printf("\r\033[K[System] %s\n> ", p.data);
            }
        
            fflush(stdout);

        }

    }
    return NULL;

    
}

int main(){

    struct sockaddr_in serv_addr;

    signal(SIGINT, handle_exit);
    sock = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        printf("Connection Failed\n"); return -1;
    }

    printf("Enter your name: ");
    fgets(username, 50, stdin);
    username[strcspn(username, "\n")] = 0;

    Packet login = {MSG_LOGIN, "", ""};
    strcpy(login.sender, username);

    if (strcmp(username, ADMIN_NAME) == 0){
        char pass[50];
        printf("Enter Password: ");
        fgets(pass, 50, stdin);
        pass[strcspn(pass, "\n")] = 0;

        if (strcmp(pass, ADMIN_PASS) != 0){
            printf("[System] Authentication Failed.\n"); 
            return 0;
        }
        login.type = MSG_ADMIN_AUTH;
        is_admin = true;
    }
    
    send(sock, &login, sizeof(Packet), 0);

    Packet res;
    recv(sock, &res, sizeof(Packet), 0);

    if (res.type == MSG_LOGIN_FAIL) {
        printf("[System] The identity '%s' is already synchronized in The Wired.\n", username);
        return 0;
    }

    if (is_admin) printf("[System] Authentication Successful. Granted Admin privileges.\n");
    else printf("--- Welcome to The Wired, %s ---\n", username);

    pthread_t tid;
    pthread_create(&tid, NULL, receive_thread, NULL);

    while(1){
        if (is_admin){
            
            printf("\n=== THE KNIGHTS CONSOLE ===\n1. Check Active Entities\n2. Check Server Uptime\n3. Emergency Shutdown\n4. Disconnect\nCommand >> ");
            int c; 
            scanf("%d", &c); 
            getchar();

            Packet p = {0, "", ""};
            strcpy(p.sender, username);

            if (c == 1) p.type = MSG_RPC_GET_USERS;
            else if (c ==2) p.type = MSG_RPC_GETUPTIME;
            else if (c ==3) p.type = MSG_RPC_SHUTDOWN;
            else {
                handle_exit(0);
            }

            send(sock, &p, sizeof(Packet), 0);
            if (c == 3) handle_exit(0);
            sleep(1);

        } else {

            char buf[BUFFER_SIZE];
            printf("> ");
            fgets(buf, BUFFER_SIZE, stdin);
            buf[strcspn(buf, "\n")] = 0;

            if (strcmp(buf, "/exit") == 0) handle_exit(0);
            
            Packet p = {MSG_CHAT, "", ""};
            strcpy(p.sender, username);
            strcpy(p.data, buf);
            send(sock, &p, sizeof(Packet), 0);
            
        }
    }
    return 0;

}