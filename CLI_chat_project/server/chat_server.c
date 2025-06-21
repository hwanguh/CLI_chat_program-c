#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 9000
#define MAX_CLIENTS 2
#define BUFFER_SIZE 1024

int server_sock;
int clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void broadcast(char *message) {
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < client_count; ++i) {
        send(clients[i], message, strlen(message), 0);
    }
    pthread_mutex_unlock(&mutex);
}

void *handle_client(void *arg) {
    int client_sock = *(int *)arg;
    char buffer[BUFFER_SIZE];

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_sock, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) break;

        printf("Message from client(%d): %s", client_sock , buffer);
        broadcast(buffer);
    }

    // 클라이언트 종료
    close(client_sock);

    pthread_mutex_lock(&mutex);
    // 리스트에서 클라이언트 제거
    for (int i = 0; i < client_count; i++) {
        if (clients[i] == client_sock) {
            clients[i] = clients[client_count - 1];
            break;
        }
    }
    client_count--;
    pthread_mutex_unlock(&mutex);

    free(arg);
    return NULL;
}

void *server_command_thread(void *arg) {
    char command[BUFFER_SIZE];
    while (1) {
        fgets(command, BUFFER_SIZE, stdin);
        if (strncmp(command, "/shutdown", 9) == 0) {
            printf("서버를 종료합니다.\n");

            // 클라이언트에게 종료 메시지 전송
            broadcast("서버가 종료되었습니다.\n");

            // 클라이언트 소켓 정리
            pthread_mutex_lock(&mutex);
            for (int i = 0; i < client_count; ++i) {
                close(clients[i]);
            }
            client_count = 0;
            pthread_mutex_unlock(&mutex);

            // 서버 소켓 종료
            close(server_sock);

            exit(0); // 전체 프로그램 종료
        }
    }
    return NULL;
}

int main() {
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_sock, MAX_CLIENTS);

    printf("채팅 서버 시작됨 포트번호 : %d\n", PORT);

    pthread_t cmd_thread;
    pthread_create(&cmd_thread, NULL, server_command_thread, NULL);

    while (1) {
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len);

        pthread_mutex_lock(&mutex);
        if (client_count >= MAX_CLIENTS) {
            printf("클라이언트 너무 많음. 연결거부됨.\n");
            close(client_sock);
            pthread_mutex_unlock(&mutex);
            continue;
        }

        clients[client_count++] = client_sock;
        pthread_mutex_unlock(&mutex);

        printf("클라이언트 연결됨: %s\n", inet_ntoa(client_addr.sin_addr));

        pthread_t tid;
        int *pclient = malloc(sizeof(int));
        *pclient = client_sock;
        pthread_create(&tid, NULL, handle_client, pclient);
        pthread_detach(tid);
    }

    close(server_sock);
    return 0;
}
