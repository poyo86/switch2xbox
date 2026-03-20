#ifndef CONTROLLER_MANAGEMENT_H
#define CONTROLLER_MANAGEMENT_H

#include <pthread.h>

typedef struct controller {
    char file_name[8];
    pthread_t controller_thread;
    struct controller *next;
} controller;

void add_controller(const char file_name[], pthread_t controller_thread);

void remove_controller(pthread_t controller_thread);

#endif
