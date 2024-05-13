#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

typedef struct msg_buffer {
    long msg_type;
    char msg_data[1024];
} message;

typedef struct {
    char username[100];
    int number;
    int error;
    char container[100];
    char image[100];
    char command[200];
    char volume[100];
} data;

void convert_data(message *send, data *client) {
    char* tok;
    int i = 0;
    tok = strtok(send->msg_data, ";");
    while (tok != 0) {
        if(i == 0) strcpy(client->username, tok);
        else if(i == 1) client->number = atoi(tok);
        else if(i == 2) client->error = atoi(tok);
        tok = strtok(0, ";");
        i++;
    }
}

int main() {
    key_t key = ftok("/tmp", 65);
    int msgid = msgget(key, 0666 | IPC_CREAT);
    int type;
    message send; data client;
    while(1) {
        printf("Masukkan username: ");
        char usn[100];
        scanf("%s", usn);
        strcpy(send.msg_data, usn);
        send.msg_type = 6;
        msgsnd(msgid, &send, sizeof(message), 0);
        msgrcv(msgid, &send, sizeof(message), 7, 0);

        convert_data(&send, &client);

        if(client.error == 1) {
            printf("Username sudah ada, masukkan lagi\n");
            continue;
        } else break;
    }

    printf("Username \"%s\" berhasil terdaftar\n", client.username);
    type = client.number + 1;

    while(1) {
        printf("\n---Data Container---\n");
        printf("Masukkan nama container: ");
        scanf("%s", client.container);
        printf("Masukkan nama image: ");
        scanf("%s", client.image);
        printf("Masukkan perintah saat kontainer dimulai: ");
        scanf("\n");
        scanf("%[^\n]s", client.command);
        printf("Masukkan volume: ");
        scanf("%s", client.volume);
        sprintf(send.msg_data, "%s;%d;%d;%s;%s;%s;%s", client.username, client.number, client.error, client.container, client.image, client.command, client.volume);

        send.msg_type = 6;
        msgsnd(msgid, &send, sizeof(message), 0);
        msgrcv(msgid, &send, sizeof(message), type, 0);
        printf("Docker compose selesai dijalankan\n\n");
    }
}
