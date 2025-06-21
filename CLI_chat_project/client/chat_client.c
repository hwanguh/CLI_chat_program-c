#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int sock;
char nickname[32];

void *send_message(void *arg) {
    char buffer[BUFFER_SIZE];
    while (1) {
        fgets(buffer, BUFFER_SIZE, stdin);
        if (strncmp(buffer, "/quit", 5) == 0) {
            close(sock);
            exit(0);
        }
        char message[BUFFER_SIZE + 32];
        snprintf(message, sizeof(message) + 3, "%s : %s", nickname, buffer);
        send(sock, message, strlen(message), 0);
    }
    return NULL;
}

void *recv_message(void *arg) {
    char buffer[BUFFER_SIZE];
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int len = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if (len <= 0) {
            printf("서버와의 연결이 종료되었습니다.\n");
            exit(0);
        }
        printf("%s", buffer);
        fflush(stdout);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    struct sockaddr_in server_addr;

    if (argc != 3) {
        printf("사용법: %s <서버IP> <포트번호>\n", argv[0]);
        return 1;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("서버 연결 실패");
        close(sock);
        exit(1);
    }
    printf("서버에 연결되었습니다. 닉네임을 입력하세요: ");
    fgets(nickname, sizeof(nickname), stdin);
    nickname[strcspn(nickname, "\n")] = '\0'; // 개행 제거

    printf("/quit 입력 시 종료됩니다.\n");

    pthread_t snd_thread, rcv_thread;
    pthread_create(&snd_thread, NULL, send_message, NULL);
    pthread_create(&rcv_thread, NULL, recv_message, NULL);

    pthread_join(snd_thread, NULL);
    pthread_join(rcv_thread, NULL);

    close(sock);
    return 0;
}
