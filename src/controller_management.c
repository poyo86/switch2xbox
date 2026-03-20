#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "controller_management.h"

extern controller *controller_list;

extern pthread_mutex_t lock;

// Adds a controller to the controller_list
void add_controller(const char file_name[], pthread_t controller_thread) {
    controller *ptr = (controller*) malloc(sizeof(controller));
    strlcpy(ptr->file_name, file_name, sizeof(ptr->file_name));
    ptr->controller_thread = controller_thread;

    pthread_mutex_lock(&lock);
    if (controller_list == NULL) {
        ptr->next = NULL;
        controller_list = ptr;
    } else {
        ptr->next = controller_list;
        controller_list = ptr;
    }
    pthread_mutex_unlock(&lock);
}

// Removes a controller from the controller_list
void remove_controller(pthread_t controller_thread) {
    pthread_mutex_lock(&lock);
    controller *ptr = controller_list;
    controller *prev = ptr;

    while (ptr != NULL && ptr->controller_thread != controller_thread) {
        prev = ptr;
        ptr = ptr->next;
    }

    if (ptr != NULL && ptr->controller_thread == controller_thread) {
        if (ptr == controller_list) controller_list = ptr->next;
        prev->next = ptr->next;
        free(ptr);
    } else {
        fprintf(stderr, "[WARNING]: Failed to release memory block\n");
    }
    pthread_mutex_unlock(&lock);
}
