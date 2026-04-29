#include "protocol.h"

typedef struct {
    int socket;
    char name[50];
    bool is_admin;
} Client;

Client clients[MAX_CLIENTS];

time_t start_time;
int server_fd;

void get_timestamp(char *buffer){

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, 20, "%Y-%m-%d %H:%M:%S", t);

}

void write_log(const char *category, const char *massage){

    FILE *f = fopen("history.log", "a");
    if (f){
        char time_buf[20];
        get_timestamp(time_buf);
        fprintf(f, "[%s] [%s] %s\n", time_buf, category, massage);
        fclose(f);
    }

}


void broadcast(Packet *p, int sender_sock){

    for (int i = 0; i < MAX_CLIENTS; i++ ){
        if(clients[i].socket != 0 && clients[i].socket != sender_sock && !clients[i].is_admin){
            send(clients[i].socket, p, sizeof(Packet), 0);
        }

    }

}

void handle_shutdown(int sig){

    write_log("System", "[EMERGENCY SHUTDOWN INITIATED]");
    printf("\n[System] Emergency Shutdown Initiated...\n");
    close(server_fd);
    exit(0);

}

int main(){

    start_time = time(NULL);
    int new_socket, max_sd, sd, activity;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    fd_set readfds;

    signal(SIGINT, handle_shutdown);

    for (int i=0; i < MAX_CLIENTS; i++){
        clients[i].socket =0;
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));

    listen(server_fd, 5);

    write_log("System", "[SERVER ONLINE]");
    printf("The Wired is running on port %d...\n", PORT);

    while(1){
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        for(int i = 0; i < MAX_CLIENTS; i++){
            sd = clients[i].socket;

            if (sd >0) FD_SET(sd, &readfds);
            if(sd> max_sd) max_sd = sd;
        }

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(server_fd, &readfds)){
            new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
            Packet p;

            if (recv(new_socket, &p, sizeof(Packet), 0) <= 0) {
                close(new_socket);
                continue; 
            }


            bool name_exist = false;
            for (int i = 0; i < MAX_CLIENTS; i++){
                if (clients[i].socket != 0 && strcmp(clients[i].name, p.sender) == 0){
                    name_exist = true;
                    break;
                }
            }

            if (name_exist){

                Packet reply = {MSG_LOGIN_FAIL, "System", ""};
                send(new_socket, &reply, sizeof(Packet), 0);
                close(new_socket);
            }

            else {
                Packet reply = {MSG_LOGIN_OK, "System", ""};
                send(new_socket, &reply, sizeof(Packet), 0);
                
                for (int i = 0; i < MAX_CLIENTS; i++){
                    if(clients[i].socket ==0){
                        clients[i].socket = new_socket;
                        strcpy(clients[i].name, p.sender);

                        clients[i].is_admin = (strcmp(p.sender, ADMIN_NAME)==0);
                        
                        char log_msg[100];
                        sprintf(log_msg, "[User '%s' connected]", p.sender);
                        write_log("System", log_msg);
                        break;

                    }
                }
            }

        }

        for (int i = 0; i < MAX_CLIENTS; i++){
            sd = clients[i].socket;
            if(FD_ISSET(sd, &readfds)){
                Packet p;
                if (recv(sd, &p, sizeof(Packet), 0) <= 0 || p.type == MSG_EXIT){

                    char log_msg[100];
                    sprintf(log_msg, "[User '%s' disconnected]", clients[i].name);
                    write_log("System", log_msg);
                    close(sd);
                    clients[i].socket = 0;

                }

                else if(p.type == MSG_CHAT){

                    char log_msg[BUFFER_SIZE + 100];
                    sprintf(log_msg, "[[%s]: %s]", p.sender, p.data);
                    write_log("User", log_msg);
                    broadcast(&p, sd);

                }

                else if(clients[i].is_admin){

                    if (p.type == MSG_RPC_GET_USERS){
                        write_log("Admin", "[RPC_GET_USERS]");
                        int count = 0;
                        for(int j = 0; j < MAX_CLIENTS;j++) if(clients[j].socket > 0 && !clients[j].is_admin) count++;

                        Packet reply = {MSG_SERVER_MSG, "System", ""};
                        sprintf(reply.data, "Active entities: %d", count);
                        send(sd, &reply, sizeof(Packet), 0);


                    }

                    else if(p.type == MSG_RPC_GETUPTIME){
                        write_log("Admin", "[RPC_GET_UPTIME]");
                        sprintf(p.data, "Server Uptime: %ld seconds", time(NULL) - start_time);

                        p.type = MSG_SERVER_MSG;
                        send(sd, &p, sizeof(Packet), 0);
                    }

                    else if (p.type == MSG_RPC_SHUTDOWN){

                        write_log("Admin", "[RPC_SHUTDOWN]");
                        handle_shutdown(0);

                    }

                }
            }
        }
        

    }

}
