#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
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
        else if(i == 3) strcpy(client->container, tok);
        else if(i == 4) strcpy(client->image, tok);
        else if(i == 5) strcpy(client->command, tok);
        else if(i == 6) strcpy(client->volume, tok);
        tok = strtok(0, ";");
        i++;
    }
}

void compose_docker(int n, data client[]) {
    FILE *compose = fopen("docker-compose.yml", "w");
    fprintf(compose, "version: '3'\nservices:\n");
    for(int i = 0; i < n; i++) {
        fprintf(compose, "   %s:\n", client[i].container);
        fprintf(compose, "      image: %s\n", client[i].image);
        fprintf(compose, "      command: %s\n", client[i].command);
        fprintf(compose, "      volumes:\n      - %s:/var/lib/volumes/\n", client[i].volume);
    }
    fprintf(compose, "volumes:\n");
    for(int i = 0; i < n; i++) {
        fprintf(compose, "   %s:\n", client[i].volume);
    }
    fclose(compose);
}

void docker_run() {
    pid_t pid = fork();
    if(pid == 0) {
        wait(NULL);
        execlp("docker", "docker", "compose", "up", "--remove-orphans", NULL);
    } else if(pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        printf("[INFO] Docker compose telah dijalankan.\n");
        pid_t pid2 = fork();
        if(pid2 == 0)
            execlp("docker", "docker", "compose", "down", NULL);
        else if(pid2 > 0) {
            waitpid(pid2, &status, 0);
            printf("[INFO] Docker compose telah dihapus.\n");
            return;
        }
    }
}

int main() {
    key_t key = ftok("/tmp", 65);
    msgctl(key, IPC_RMID, NULL);
    int msgid = msgget(key, 0666 | IPC_CREAT);

    printf("Selamat datang Ben White pintar!\n");
    int n;
    do {
        printf("Buruan ketik banyak client: ");
        scanf("%d", &n);
    } while (n < 1 || n > 5);

    message client;
    data client_data[n];
    for(int i = 0; i < n; i++) {
        while(1) {
            msgrcv(msgid, &client, sizeof(message), 6, 0);
            strcpy(client_data[i].username, client.msg_data);
            client_data[i].number = i;
            client_data[i].error = 0;
            client.msg_type = 7;
            sprintf(client.msg_data, "%s;%d;%d", client_data[i].username, client_data[i].number, client_data[i].error);
            if(i == 0) break;

            for(int j = 0; j < i; j++) {
                if(strcmp(client_data[i].username, client_data[j].username) == 0) {
                    client_data[i].error = 1;
                    sprintf(client.msg_data, "%s;%d;%d", client_data[i].username, client_data[i].number, client_data[i].error);
                    msgsnd(msgid, &client, sizeof(message), 0);
                }
            }

            if(client_data[i].error == 0) {
                break;
            } else {
                continue;
            }
        }
        for(int i = 0; i < n; i++) {
            msgrcv(msgid, &client, sizeof(message), 6, 0);
            data temp;
            convert_data(&client, &temp);
            int num = temp.number;
            printf("[INFO] Masuk data container dari Client %d\n", num + 1);
            client_data[num] = temp;
        }
        {
            msgrcv(msgid, &client, sizeof(message), 6, 0);
            data temp;
            convert_data(&client, &temp);
            int num = temp.number;
            printf("[INFO] Masuk data container dari Client %d\n", num + 1);
            client_data[num] = temp;
        }
        compose_docker(n, client_data);
        printf("[INFO] Docker compose telah dibuat.\n");

        docker_run();

        for(int i = 1; i <= n; i++) {
            client.msg_type = i;
            msgsnd(msgid, &client, sizeof(message), 0);
        }
    }
}
