#define _GNU_SOURCE

#include "image_server.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>
#include "bmp.h"

#define MAX_READ_BYTES 1024
#define BLOCK_SIZE 1024

#define CONNECTION_QUEUE_LEN 1

bool image_server_started = false;
pthread_t image_server_thread;
pthread_mutex_t image_server_mutex = PTHREAD_MUTEX_INITIALIZER;
size_t image_server_data_size = 0;
uint8_t *image_server_data = NULL;

int start_server(char *port) {
    struct addrinfo hints = {0};
    struct addrinfo *info;

    hints.ai_family = AF_INET; // ipv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_protocol = IPPROTO_TCP; // TCP, obviously
    hints.ai_flags = AI_PASSIVE; // server

    // find a connection configuration that fits our description
    if (getaddrinfo(NULL, port, &hints, &info) != 0) {
        perror("getaddrinfo()");
        exit(1);
    }

    int server_num = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
    if (server_num < 0) {
        perror("socket()");
        exit(1);
    }

    // lets the program immediately rebind the socket after quitting
    // otherwise there is a wait time and bind will fail with "Address already in use"
    int enable = 1;
    if (setsockopt(server_num, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR)");
        exit(1);
    }

    if (bind(server_num, info->ai_addr, info->ai_addrlen) < 0) {
        perror("bind()");
        exit(1);
    }

    freeaddrinfo(info);

    if (listen(server_num, CONNECTION_QUEUE_LEN) != 0) {
        perror("listen()");
        exit(1);
    }

    return server_num;
}

//client connection
void respond(int connection_desc) {
    char mesg[MAX_READ_BYTES] = {0};

    int read_status = read(connection_desc, mesg, sizeof(mesg));

    if (read_status < 0) {
        fprintf(stderr, "read() error\n");
    } else if (read_status == 0) {
        // fprintf(stderr, "Client disconnected unexpectedly.\n");
    } else {
        // printf("%s\n", mesg);
        static const char get_str[] = "GET /image.bmp";
        if (strncmp(mesg, get_str, sizeof(get_str) - 1) == 0 &&
            image_server_data_size > 0) {
            char response[] = "HTTP/1.0 200 OK\n"
                              "Content-Type: image/bmp;\n\n";
            send(connection_desc, response, sizeof(response) - 1, MSG_NOSIGNAL);

            pthread_mutex_lock(&image_server_mutex);
            send(connection_desc, image_server_data, image_server_data_size, MSG_NOSIGNAL);
            pthread_mutex_unlock(&image_server_mutex);
        } else {
            char response[] = "HTTP/1.0 404 Not Found\n"
                              "Content-Type: text/plain;\n\n";
            send(connection_desc, response, sizeof(response) - 1, MSG_NOSIGNAL);
        }
    }

    shutdown(connection_desc, SHUT_RDWR); // All further send and recieve operations are DISABLED...
    close(connection_desc);
}

void *image_server_run(void *user) {
    signal(SIGPIPE, SIG_IGN);

    char *port = user;
    fprintf(stderr, "Serving images at http://localhost:%s/image.bmp\n", port);

    int server_num = start_server(port);
    while (1) {
        int connection_desc = accept(server_num, NULL, NULL);
        if (connection_desc < 0) {
            perror("accept()");
        } else {
            respond(connection_desc);
        }
    }

    return NULL;
}

void image_server_start(char *port) {
    if (!image_server_started) {
        image_server_started = true;
        pthread_create(&image_server_thread, NULL, image_server_run, port);
    }
}

void image_server_set_data(size_t size, uint8_t *data) {
    pthread_mutex_lock(&image_server_mutex);

    if (size != image_server_data_size) {
        image_server_data_size = size;
        image_server_data = realloc(image_server_data, size);
    }

    memcpy(image_server_data, data, size);

    pthread_mutex_unlock(&image_server_mutex);
}
