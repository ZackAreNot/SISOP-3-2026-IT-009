# Laporan Praktikum Sistem Operasi

> Nama : Maulana Zaki Putra Zakaria

> NRP : 5027251009

> Asisten : MOO

## Soal 1: The Knights Console

### Deskripsi Singkat
Pada soal pertama ini, kita ditugaskan untuk merancang sebuah sistem aplikasi *chat* atau *console* berbasis *Client-Server* menggunakan *Socket Programming* di bahasa C. Sistem yang diberi nama "The Wired" ini memungkinkan banyak entitas (klien) untuk terhubung secara bersamaan dan saling bertukar pesan (fitur *broadcasting*). Keunikan dari sistem ini adalah adanya sistem peran (*role-based*), di mana terdapat peran "Administrator" dengan hak istimewa khusus. Jika seorang klien berhasil melakukan otentikasi masuk menggunakan *username* dan *password* rahasia, ia akan mendapatkan akses ke "The Knights Console". Dari *console* ini, Admin tidak bertukar pesan dengan entitas lain, melainkan dapat mengirimkan perintah *Remote Procedure Call* (RPC) ke server, seperti mengecek jumlah entitas yang aktif, melihat *uptime* (lama server menyala), hingga melakukan *Emergency Shutdown* untuk mematikan server secara paksa.

---

### Penjelasan Kode: 1. File `protocol.h`

File `protocol.h` bertindak sebagai "kontrak" atau kesepakatan aturan komunikasi antara *Client* (`navi.c`) dan *Server* (`wired.c`)[cite: 2]. Karena menggunakan bahasa C, semua definisi struktur paket, tipe pesan, dan konfigurasi jaringan harus dideklarasikan di satu tempat agar kedua program memiliki pemahaman yang sama terhadap data yang dikirim dan diterima melalui jaringan[cite: 2].

#### 1. Include Guards dan Library
```c
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <signal.h>
#include <time.h>
#include <stdbool.h>
```
**Penjelasan:**
*   **Include Guards (`#ifndef PROTOCOL_H`)**: Baris `#ifndef` dan awalan `#define` ini berfungsi mencegah terjadinya *double inclusion* (penyertaan ganda)[cite: 2]. Jika file *client* atau *server* memanggil file ini lebih dari sekali, *compiler* tidak akan memunculkan *error* karena blok kode hanya akan dieksekusi satu kali oleh sistem.
*   **Library Jaringan dan Sistem**: Mengimpor *library* POSIX yang esensial. `<sys/socket.h>` dan `<arpa/inet.h>` sangat krusial untuk melakukan *socket programming* TCP/IP di C[cite: 2]. `<pthread.h>` digunakan untuk *multithreading* di sisi klien[cite: 2], sementara `<sys/select.h>` dipakai untuk *I/O Multiplexing* di sisi server[cite: 2]. `<signal.h>` dipakai untuk menangani interupsi paksa saat program dihentikan[cite: 2].

#### 2. Konfigurasi Makro
```c
#define PORT 8080
#define MAX_CLIENTS 30
#define BUFFER_SIZE 1024
#define ADMIN_NAME "The Knights"
#define ADMIN_PASS "protocol7"
```
**Penjelasan:**
Bagian ini mendefinisikan konstanta global (*hardcoded*) agar nilai-nilai penting mudah dimodifikasi dari satu titik.
*   **`PORT 8080`**: Pintu gerbang jaringan tempat server mendengarkan koneksi masuk[cite: 2].
*   **`MAX_CLIENTS 30`**: Membatasi kapasitas server "The Wired" agar hanya menampung maksimal 30 klien secara bersamaan[cite: 2].
*   **`BUFFER_SIZE 1024`**: Mengatur batas panjang maksimal pesan teks (*payload*) yang bisa dikirimkan dalam satu waktu sebesar 1024 byte[cite: 2].
*   **`ADMIN_NAME` dan `ADMIN_PASS`**: Menyimpan kredensial rahasia untuk proses *login* sebagai admin, yang disetel sebagai "The Knights" dan "protocol7"[cite: 2].

#### 3. Enumerasi Tipe Pesan (MsgType)
```c
typedef enum {
    MSG_LOGIN, MSG_LOGIN_OK, MSG_LOGIN_FAIL, 
    MSG_CHAT, MSG_ADMIN_AUTH, MSG_RPC_GET_USERS,
    MSG_RPC_GETUPTIME, MSG_RPC_SHUTDOWN, MSG_EXIT, MSG_SERVER_MSG
} MsgType;
```
**Penjelasan:**
Sistem *enum* ini adalah inti dari protokol IPC/RPC yang kita buat[cite: 2]. Karena server harus membedakan dengan cepat mana pesan *chat* dan mana perintah sistem, setiap data yang dikirim akan diberi "label" `MsgType`[cite: 2]:
*   **Proses Otentikasi**: `MSG_LOGIN` (untuk klien biasa), `MSG_ADMIN_AUTH` (untuk admin), `MSG_LOGIN_OK` (jika diterima), dan `MSG_LOGIN_FAIL` (jika nama sudah ada)[cite: 2].
*   **Komunikasi & Sinyal Keluar**: `MSG_CHAT` digunakan untuk pesan antar entitas, sedangkan `MSG_EXIT` digunakan klien untuk memberi tahu server bahwa ia memutus koneksi[cite: 2].
*   **Fungsi RPC**: `MSG_RPC_GET_USERS`, `MSG_RPC_GETUPTIME`, dan `MSG_RPC_SHUTDOWN` adalah label instruksi khusus yang hanya bisa dikirimkan oleh Admin dari dalam *console*[cite: 2].
*   **Balasan Sistem**: `MSG_SERVER_MSG` dilabelkan pada balasan atau notifikasi yang datang langsung dari server[cite: 2].

#### 4. Struktur Data Paket (Packet)
```c
typedef struct {
    MsgType type;
    char sender[50];
    char data[BUFFER_SIZE];
} Packet;

#endif
```
**Penjelasan:**
Protokol TCP mengirimkan data dalam bentuk aliran *byte* (*byte stream*) yang abstrak. Agar tidak berantakan, data tersebut harus selalu dibungkus ke dalam *struct* `Packet` yang terstandardisasi ini[cite: 2]. Setiap perintah pengiriman (seperti `send()` dan `recv()`) dijamin menggunakan format yang sama[cite: 2].
*   **`type`**: Menyimpan jenis/label pesan berdasarkan `MsgType` di atas[cite: 2].
*   **`sender[50]`**: Menyimpan nama (identitas) klien yang mengirimkan pesan tersebut[cite: 2].
*   **`data[BUFFER_SIZE]`**: Menyimpan isi konten utama, entah itu teks *chat* biasa atau teks informasi yang dikembalikan oleh sistem[cite: 2].
*   **`#endif`**: Merupakan penutup dari blok pengecekan *Include Guards* di awal file tadi[cite: 2].


***

### Penjelasan Kode: 2. File `navi.c` (Sisi Klien)

File `navi.c` adalah program *client* yang akan dieksekusi oleh setiap pengguna (entitas) untuk terhubung ke dalam The Wired[cite: 1]. Program ini mengimplementasikan *multithreading* agar pengguna dapat mengetik pesan sekaligus menerima pesan dari pengguna lain secara bersamaan tanpa saling memblokir (menunggu) satu sama lain[cite: 1].

#### 1. Deklarasi Global dan Penanganan Exit (Keluar)
```c
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
```
**Penjelasan:**
*   **Variabel Global**: Di sini kita mendefinisikan *socket descriptor* (`sock`), nama pengguna (`username`), dan sebuah *flag* penanda peran (`is_admin`)[cite: 1]. Variabel ini dibuat global karena akan diakses oleh *thread* utama maupun *thread* penerima pesan secara bersamaan[cite: 1].
*   **Fungsi `handle_exit`**: Ini adalah fungsi *signal handler* yang akan dipanggil saat pengguna menekan `Ctrl+C` atau mengetik perintah keluar[cite: 1]. Alih-alih langsung mematikan program, fungsi ini "berpamitan" dengan sopan ke server dengan mengirimkan paket berlabel `MSG_EXIT`[cite: 1]. Setelah itu, koneksi *socket* diputus (`close(sock)`) secara aman sebelum mematikan program[cite: 1].

#### 2. Thread Penerima Pesan (Asynchronous Listening)
```c
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
```
**Penjelasan:**
*   Fungsi ini akan dijalankan di *thread* latar belakang (*background*)[cite: 1]. Loop `while` akan terus memblokir dan mendengarkan data yang masuk melalui fungsi `recv()`[cite: 1].
*   **Kondisi `MSG_CHAT`**: Jika pesan yang masuk bertipe *chat* biasa, program akan mencetaknya ke layar[cite: 1].
*   **Kondisi `MSG_SERVER_MSG`**: Jika pesan berasal dari balasan sistem (seperti respons perintah RPC Admin), program akan menggunakan format `[System]`[cite: 1]. Perhatikan bahwa *prompt* untuk Admin (spasi kosong) berbeda dengan pengguna biasa (`> `)[cite: 1].
*   **Trik Pembersihan UI (`\r\033[K`)**: Ini adalah karakter *escape* khusus terminal[cite: 1]. Fungsinya untuk menghapus baris input yang sedang ditik pengguna saat ini, menampilkan pesan yang baru masuk, lalu memunculkan kembali *prompt* pengetikan (`> `) di bawahnya[cite: 1]. Ini mencegah tampilan teks menjadi bertumpuk dan kacau.

#### 3. Inisialisasi Koneksi Socket (Fungsi Main)
```c
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
```
**Penjelasan:**
*   **`signal(SIGINT, handle_exit)`**: Menyambungkan deteksi tombol `Ctrl+C` dari pengguna agar memicu fungsi *graceful exit* yang sudah dibuat di atas[cite: 1].
*   **`socket(...)`**: Membuat *endpoint socket* menggunakan protokol IPv4 (`AF_INET`) dan TCP (`SOCK_STREAM`)[cite: 1].
*   **`inet_pton(...)` dan `connect(...)`**: Mengonversi alamat IP server "127.0.0.1" (Localhost) menjadi bentuk biner, kemudian mencoba menjalin koneksi TCP secara aktif ke server The Wired pada port 8080[cite: 1].

#### 4. Proses Login dan Evaluasi Kredensial
```c
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
```
**Penjelasan:**
*   **Input Username**: Meminta input nama entitas. Fungsi `strcspn` digunakan untuk membuang karakter *enter* (`\n`) terbawa oleh fungsi `fgets`[cite: 1].
*   **Pengecekan *Privilege* Admin**: Jika nama yang dimasukkan adalah "The Knights", program akan langsung menodongkan *prompt* *password*[cite: 1]. Jika *password* yang dimasukkan adalah "protocol7", tipe paket diubah dari otentikasi biasa (`MSG_LOGIN`) menjadi `MSG_ADMIN_AUTH` dan variabel `is_admin` diaktifkan[cite: 1].
*   **Respons Otentikasi**: Setelah mengirimkan paket masuk, klien menunggu jawaban server. Jika server membalas dengan `MSG_LOGIN_FAIL` (biasanya karena nama tersebut sedang *online*), program klien akan otomatis tertutup[cite: 1].

#### 5. Interface Pengguna dan Main Loop
```c
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
            else { handle_exit(0); }

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
```
**Penjelasan:**
*   **Aktivasi Thread**: Baris `pthread_create` memanggil fungsi `receive_thread` agar berjalan paralel secara mandiri di latar belakang[cite: 1].
*   **Logika Menu Admin (The Knights Console)**: Jika pengguna adalah Admin, layar berubah menjadi menu interaktif berbasis angka[cite: 1]. Pemilihan menu (1, 2, atau 3) tidak akan dicetak ke layar *chat*, melainkan langsung dibungkus menjadi tipe perintah `MSG_RPC_*` yang tersembunyi dan ditembakkan ke server[cite: 1]. Jika Admin menekan angka 3 (Shutdown), program Admin juga akan ikut bunuh diri (`handle_exit`)[cite: 1].
*   **Logika Mode Pesan Biasa**: Jika bukan Admin, pengguna dikunci dalam layar teks biasa. Apapun yang ia ketik akan dibungkus sebagai `MSG_CHAT` dan dikirim untuk di-*broadcast* ke semua orang oleh server[cite: 1]. Ada fitur *escape word* rahasia, di mana jika *user* mengetik persis `/exit`, maka program akan dihentikan[cite: 1].

***

### Penjelasan Kode: 3. File `wired.c` (Sisi Server)

File `wired.c` bertindak sebagai *Central Server* yang bertugas mengatur lalu lintas jaringan dari semua entitas[cite: 3]. Program ini menggunakan pendekatan *I/O Multiplexing* berbasis `select()`, yang memungkinkannya melayani puluhan klien (`MAX_CLIENTS`) secara bersamaan dalam satu *thread* utama tanpa saling memblokir (*non-blocking I/O model*)[cite: 3]. Selain itu, server juga bertindak sebagai pencatat riwayat (*logger*) dari semua aktivitas yang terjadi di dalam jaringan[cite: 3].

#### 1. Struktur Data Klien dan Fungsi Pencatatan Log (Logging)
```c
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
```
**Penjelasan:**
*   **Struktur `Client`**: Sebuah wadah data khusus untuk menyimpan identitas setiap entitas yang terhubung[cite: 3]. Struktur ini menyimpan *File Descriptor* dari *socket* klien (`socket`), nama pengguna (`name`), dan label apakah ia memiliki akses admin (`is_admin`)[cite: 3]. Array `clients[MAX_CLIENTS]` digunakan sebagai basis data memori sementara untuk mencatat siapa saja yang sedang *online*[cite: 3].
*   **Variabel Waktu (`start_time`)**: Digunakan untuk merekam kapan tepatnya server pertama kali dinyalakan. Variabel ini krusial untuk fitur pengecekan *Server Uptime* nantinya[cite: 3].
*   **Fungsi `get_timestamp` dan `write_log`**: Server ditugaskan untuk melakukan *tracking* aktivitas[cite: 3]. `get_timestamp` mengambil waktu dari sistem (OS) dan mengubahnya ke dalam format string (Tahun-Bulan-Tanggal Jam:Menit:Detik)[cite: 3]. Kemudian `write_log` akan menggunakan waktu tersebut untuk menulis sebuah baris aktivitas ke dalam file fisik bernama `history.log` menggunakan mode `"a"` (*append*), agar data sebelumnya tidak terhapus[cite: 3].

#### 2. Mekanisme Broadcast dan Pengamanan Sistem (Shutdown)
```c
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
```
**Penjelasan:**
*   **Fungsi `broadcast`**: Ini adalah mesin pengirim pesan massal[cite: 3]. Saat ada klien mengirim pesan *chat*, server akan melakukan *looping* ke seluruh isi array `clients`[cite: 3]. Server memastikan pesan dikirim ke *socket* yang aktif (`!= 0`), **TIDAK** dikirim balik ke si pengirim asli (`!= sender_sock`), dan **TIDAK** dikirim ke Admin (`!is_admin`) karena *The Knights Console* harus bebas dari *spam* obrolan biasa[cite: 3].
*   **Fungsi `handle_shutdown`**: *Signal handler* khusus server. Saat dipicu (entah via `Ctrl+C` di terminal atau lewat instruksi RPC Admin), server akan segera menulis status ke dalam log historis, mematikan gerbang *socket* utama (`server_fd`), dan keluar dari program dengan aman[cite: 3].

#### 3. Inisialisasi TCP Server
```c
int main(){
    start_time = time(NULL);
    int new_socket, max_sd, sd, activity;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    fd_set readfds;

    signal(SIGINT, handle_shutdown);

    for (int i=0; i < MAX_CLIENTS; i++) clients[i].socket = 0;

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
```
**Penjelasan:**
*   **Persiapan Array**: Mengisi seluruh *slot* klien dengan `0` di awal agar sistem tahu bahwa belum ada pengguna yang menempati *slot* tersebut[cite: 3].
*   **`setsockopt(..., SO_REUSEADDR, ...)`**: Ini adalah parameter perbaikan yang sangat penting. Seringkali ketika server mati dan langsung dinyalakan lagi, *port* masih "terkunci" oleh OS (muncul *error* "Address already in use")[cite: 3]. Fungsi ini memaksa OS untuk melepaskan port tersebut agar bisa langsung digunakan ulang[cite: 3].
*   **Bind & Listen**: Mengikat (*binding*) server ke IP publik sistem (`INADDR_ANY`) dan *port* 8080, lalu masuk ke mode *listening* (menunggu kedatangan tamu)[cite: 3]. Server juga akan mencatat riwayat pertamanya ke `history.log` bahwa sistem sudah *online*[cite: 3].

#### 4. I/O Multiplexing Loop (Jantung Server)
```c
    while(1){
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        for(int i = 0; i < MAX_CLIENTS; i++){
            sd = clients[i].socket;
            if (sd > 0) FD_SET(sd, &readfds);
            if (sd > max_sd) max_sd = sd;
        }

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
```
**Penjelasan:**
Di sinilah letak keajaiban *select()* berada. Alih-alih melayani klien satu per satu secara bergantian (yang membuat klien lain mengantre panjang), fungsi ini akan "menyasap" semua saluran komunikasi sekaligus[cite: 3].
*   **`FD_ZERO` dan `FD_SET`**: Mengosongkan himpunan pendengar (*file descriptor set*), lalu mendaftarkan *socket* gerbang utama (`server_fd`) dan seluruh *socket* klien yang sedang aktif ke dalam himpunan pendengar tersebut[cite: 3].
*   **`select(...)`**: Fungsi ini akan menyebabkan program "tertidur" di baris tersebut. Ia baru akan terbangun secara otomatis HANYA JIKA ada aktivitas fisik (ada klien baru masuk, atau ada klien yang mengirim data teks) pada salah satu *socket* yang ada di dalam himpunan tadi[cite: 3]. Hal ini jauh lebih hemat CPU daripada melakukan *looping* terus-menerus.

#### 5. Menangani Entitas Masuk (New Connection & Login)
```c
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
                    name_exist = true; break;
                }
            }

            if (name_exist){
                Packet reply = {MSG_LOGIN_FAIL, "System", ""};
                send(new_socket, &reply, sizeof(Packet), 0);
                close(new_socket);
            } else {
                Packet reply = {MSG_LOGIN_OK, "System", ""};
                send(new_socket, &reply, sizeof(Packet), 0);
                
                for (int i = 0; i < MAX_CLIENTS; i++){
                    if(clients[i].socket == 0){
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
```
**Penjelasan:**
*   Jika yang bergetar/beraktivitas adalah `server_fd` (gerbang utama), berarti ada pengguna baru yang ingin terhubung[cite: 3]. Server langsung melakukan `accept` dan menangkap data otentikasi pertamanya[cite: 3].
*   **Validasi Identitas Unik**: Server akan melakukan *scanning* pada array `clients`. Jika nama yang diajukan oleh pengguna baru itu sudah ada yang memakai dan orangnya sedang aktif (`name_exist == true`), server akan menolaknya dengan paket balasan `MSG_LOGIN_FAIL` lalu memutus *socket* tersebut[cite: 3].
*   **Registrasi Berhasil**: Jika namanya aman, server akan membalas dengan `MSG_LOGIN_OK`, memasukkan identitasnya (nama, koneksi, status admin) ke dalam *slot* kosong pertama di memori array `clients`, dan langsung mencetak rekam jejak koneksinya ke log historis[cite: 3].

#### 6. Memproses Interaksi Klien (Chat & Remote Procedure Call)
```c
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
```
**Penjelasan:**
Jika yang beraktivitas adalah `sd` (salah satu *socket* milik klien yang sudah terhubung), maka server akan membaca paket pesan (Packet) yang dikirimkan oleh mereka[cite: 3].
*   **Deteksi Putus Koneksi (`MSG_EXIT`)**: Jika klien keluar secara sengaja via perintah di terminal atau mati mendadak (`recv` bernilai 0), server akan me-reset *slot* memorinya (`clients[i].socket = 0`), menutup saluran `close(sd)`, dan merekam riwayat *disconnected* ke log historis[cite: 3].
*   **Deteksi Mode Komunikasi (`MSG_CHAT`)**: Jika pesan yang ditangkap adalah obrolan antar pengguna, server akan merekam isi *chat* tersebut ke log, lalu melemparkannya ke fungsi `broadcast()` agar tersebar ke seluruh entitas yang lain[cite: 3].
*   **Deteksi Mode Administrator**: Jika klien berstatus Admin, server mengizinkan pembacaan perintah khusus (*RPC Execution*)[cite: 3].
    *   Jika `MSG_RPC_GET_USERS`, server akan menghitung sisa slot yang tidak bernilai nol, dan mengirimkan balik angkanya menggunakan format `MSG_SERVER_MSG` khusus ke layar admin saja[cite: 3].
    *   Jika `MSG_RPC_GETUPTIME`, server akan mengurangi waktu saat ini dengan `start_time` di memori, merubahnya ke dalam format jumlah detik, lalu membalas ke layar admin[cite: 3].
    *   Jika `MSG_RPC_SHUTDOWN`, admin menggunakan *ultimate skill*-nya. Server memanggil fungsi `handle_shutdown(0)` yang langsung menyapu bersih dan menonaktifkan sistem secara total tanpa perlawanan[cite: 3].

***

### Output / Dokumentasi Uji Coba (Test Case & Error Handling)

Berikut adalah hasil pengujian dari berbagai skenario fungsionalitas dan penanganan *error* (*error handling*) pada program *Client* (`navi.c`) dan *Server* (`wired.c`):

#### 1. Uji Coba: Koneksi Awal dan Login Klien Biasa
Pada skenario ini, beberapa klien (entitas) diuji untuk terhubung ke server "The Wired"[cite: 1, 3].
*   **[Masukkan Screenshot Klien Berhasil Login di Sini]**
*   **Keterangan:** Saat *client* dijalankan dan memasukkan nama (selain "The Knights"), server akan menerima koneksi, mendaftarkannya ke dalam *array* klien, dan klien mendapatkan pesan `"--- Welcome to The Wired, [Nama] ---"`[cite: 1, 3]. Di sisi server, aktivitas ini tercatat pada file `history.log` sebagai `[User '[Nama]' connected]`[cite: 3].

#### 2. Error Handling: Username Duplikat (Sudah Digunakan)
Sistem memiliki pengamanan agar tidak ada dua entitas dengan nama yang sama di dalam satu jaringan[cite: 3].
*   **[Masukkan Screenshot Error Username Duplikat di Sini]**
*   **Keterangan:** Klien mencoba masuk menggunakan nama yang sedang aktif digunakan oleh klien lain[cite: 3]. Server mendeteksi duplikasi ini melalui *looping* pengecekan, kemudian mengirimkan paket balasan `MSG_LOGIN_FAIL`[cite: 3]. Klien secara otomatis menampilkan pesan error `[System] The identity '[Nama]' is already synchronized in The Wired.` dan program klien langsung ditutup untuk mencegah konflik[cite: 1].

#### 3. Uji Coba: Otentikasi Administrator dan Password Salah
Pengujian sistem hak istimewa (*privilege*) menggunakan nama "The Knights"[cite: 1].
*   **[Masukkan Screenshot Login Admin Gagal dan Berhasil di Sini]**
*   **Keterangan (Error Handling):** Jika *user* menggunakan nama "The Knights" namun salah memasukkan *password* (bukan "protocol7"), program akan memunculkan pesan `[System] Authentication Failed.` dan langsung memutuskan koneksi secara sepihak sebelum paket otentikasi dikirim ke server[cite: 1].
*   **Keterangan (Sukses):** Jika *password* benar, *user* diakui sebagai Admin dan terminal langsung berubah memunculkan antarmuka khusus `=== THE KNIGHTS CONSOLE ===`[cite: 1].

#### 4. Uji Coba: Fitur Broadcasting (Chatting)
Skenario pertukaran pesan antar pengguna biasa secara *real-time*[cite: 1, 3].
*   **[Masukkan Screenshot Chatting Antar 2-3 Klien di Sini]**
*   **Keterangan:** Entitas A mengirim pesan, lalu Entitas B dan C akan menerimanya secara *real-time*[cite: 1, 3]. Penanganan *output* menggunakan kode *escape terminal* `\r\033[K` bekerja dengan baik, sehingga pesan masuk tidak merusak tampilan baris input `> ` milik klien yang sedang mengetik[cite: 1]. Pesan *chat* ini juga tidak masuk ke layar Admin, sesuai dengan filter di logika `broadcast` server[cite: 3].

#### 5. Uji Coba: Eksekusi Remote Procedure Call (RPC)
Pengujian eksekusi perintah khusus dari *console* Admin ke server[cite: 1, 3].
*   **[Masukkan Screenshot Admin Menjalankan Menu 1 dan 2 di Sini]**
*   **Keterangan:** Admin menekan angka 1 (`Check Active Entities`), server membalas dengan total jumlah klien yang sedang terhubung[cite: 1, 3]. Admin menekan angka 2 (`Check Server Uptime`), server membalas dengan kalkulasi waktu menyala server (dalam satuan detik)[cite: 1, 3].

#### 6. Error Handling: Graceful Disconnect dan Emergency Shutdown
Sistem harus bisa menangani penutupan koneksi secara rapi tanpa membuat server *crash*[cite: 1, 3].
*   **[Masukkan Screenshot Client /exit dan Admin Shutdown di Sini]**
*   **Keterangan (Client Exit):** Jika *user* mengetik `/exit` atau menekan `Ctrl+C`, *signal handler* akan mengirimkan paket `MSG_EXIT` ke server[cite: 1]. Server kemudian membersihkan memori *slot* pengguna tersebut dan mencatatnya sebagai `[disconnected]` di log[cite: 3].
*   **Keterangan (Admin Shutdown):** Admin menekan angka 3 pada *console* (Emergency Shutdown). Server menerima paket `MSG_RPC_SHUTDOWN`, mencatat status darurat di log, lalu mengeksekusi `close()` pada *socket* utama[cite: 1, 3]. Hal ini mematikan server secara total, dan klien-klien yang tersambung akan kehilangan koneksi secara otomatis.

---

### Kendala dalam Pengerjaan Soal 1

Dalam proses pengerjaan pembuatan arsitektur jaringan *Client-Server* "The Wired" ini, terdapat beberapa kendala dan tantangan teknis yang cukup memakan waktu, antara lain:

1. **Pemahaman Konsep I/O Multiplexing (`select`):** 
   Berbeda dengan pendekatan *multithreading* di mana setiap klien dibuatkan satu *thread* khusus di server, penggunaan `select()` cukup membingungkan pada awalnya. Mengatur himpunan *File Descriptor* (`fd_set`) dan membedakan antara aktivitas masuknya klien baru (`server_fd`) dengan aktivitas masuknya pesan dari klien lama (`clients[i].socket`) memerlukan ketelitian tinggi agar tidak terjadi *segmentation fault* atau koneksi yang menggantung (*hanging*).
2. **Tabrakan UI/UX pada Terminal Klien:** 
   Kendala terbesar di sisi klien adalah saat sedang asyik mengetik pesan panjang, tiba-tiba ada pesan masuk dari klien lain yang merusak teks yang sedang diketik (terminal menjadi berantakan). Untuk mengakalinya, butuh waktu eksplorasi untuk menemukan trik menggunakan karakter *escape sequence* bawaan terminal (`\r\033[K`) yang berfungsi menghapus paksa baris pengetikan sesaat, menge-*print* pesan masuk, dan me-*restore* kembali *prompt* pengetikan (*Asynchronous UI*).
3. **Isu "Address Already in Use" pada Socket:** 
   Saat melakukan tahap pengujian (*debugging*), server sering dimatikan secara paksa dan langsung dinyalakan kembali. Hal ini seringkali memunculkan *error bind* karena port 8080 masih ditahan oleh Sistem Operasi. Kendala ini akhirnya terselesaikan dengan menambahkan fungsi `setsockopt` beserta parameter `SO_REUSEADDR` agar port bisa di-*recycle* paksa oleh server.
4. **Sinkronisasi Data Struct (Packet):**
   Mendesain *protocol.h* butuh kehati-hatian. Awalnya sering terjadi pengiriman pesan yang terpotong atau tercampur karena ketidaksesuaian ukuran *buffer* antara fungsi `send` dan `recv`. Hal ini diperbaiki dengan menyatukan seluruh format pengiriman ke dalam satu tipe data baku `Packet` yang ukurannya sudah diketahui pasti oleh *compiler*.

***

