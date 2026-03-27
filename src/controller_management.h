// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef CONTROLLER_MANAGEMENT_H
#define CONTROLLER_MANAGEMENT_H

#include <pthread.h>
#include <libevdev-1.0/libevdev/libevdev-uinput.h>

typedef struct controller {
    char file_name[8];
    pthread_t controller_thread;
    struct controller *next;
} controller;

void add_controller(const char file_name[], pthread_t controller_thread);

void remove_controller(pthread_t controller_thread);

void create_xbox_controller(struct libevdev_uinput **uidev);

#endif
