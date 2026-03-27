// SPDX-License-Identifier: GPL-3.0-or-later

#define MICROSOFT_VENDOR_ID 0x45e
#define MICROSOFT_XBOX_360 0x28e

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <libevdev-1.0/libevdev/libevdev-uinput.h>

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

void create_xbox_controller(struct libevdev_uinput **uidev) {
    struct libevdev *dev = libevdev_new();

    libevdev_set_id_vendor(dev, MICROSOFT_VENDOR_ID);
    libevdev_set_id_product(dev, MICROSOFT_XBOX_360);
    libevdev_set_name(dev, "Microsoft X-Box 360 pad");

    libevdev_enable_event_type(dev, EV_SYN);

    libevdev_enable_event_type(dev, EV_KEY);
    libevdev_enable_event_code(dev, EV_KEY, BTN_SOUTH, NULL);
    libevdev_enable_event_code(dev, EV_KEY, BTN_EAST, NULL);
    libevdev_enable_event_code(dev, EV_KEY, BTN_NORTH, NULL);
    libevdev_enable_event_code(dev, EV_KEY, BTN_WEST, NULL);
    libevdev_enable_event_code(dev, EV_KEY, BTN_TL, NULL);
    libevdev_enable_event_code(dev, EV_KEY, BTN_TR, NULL);
    libevdev_enable_event_code(dev, EV_KEY, BTN_SELECT, NULL);
    libevdev_enable_event_code(dev, EV_KEY, BTN_START, NULL);
    libevdev_enable_event_code(dev, EV_KEY, BTN_MODE, NULL);
    libevdev_enable_event_code(dev, EV_KEY, BTN_THUMBL, NULL);
    libevdev_enable_event_code(dev, EV_KEY, BTN_THUMBR, NULL);

    struct input_absinfo abs_x, *abs_y, *abs_rx, *abs_ry;
    abs_x.value = 0;
    abs_x.minimum = -32768;
    abs_x.maximum = 32767;
    abs_x.fuzz = 16;
    abs_x.flat = 128;
    abs_x.resolution = 0;

    abs_y = abs_rx = abs_ry = &abs_x;

    struct input_absinfo abs_z, *abs_rz;
    abs_z.value = 0;
    abs_z.minimum = 0;
    abs_z.maximum = 255;
    abs_z.fuzz = 0;
    abs_z.flat = 0;
    abs_z.resolution = 0;

    abs_rz = &abs_z;

    struct input_absinfo abs_hat0x, *abs_hat0y;
    abs_hat0x.value = 0;
    abs_hat0x.minimum = -1;
    abs_hat0x.maximum = 1;
    abs_hat0x.fuzz = 0;
    abs_hat0x.flat = 0;
    abs_hat0x.resolution = 0;

    abs_hat0y = &abs_hat0x;

    libevdev_enable_event_type(dev, EV_ABS);
    libevdev_enable_event_code(dev, EV_ABS, ABS_X, &abs_x);
    libevdev_enable_event_code(dev, EV_ABS, ABS_Y, abs_y);
    libevdev_enable_event_code(dev, EV_ABS, ABS_RX, abs_rx);
    libevdev_enable_event_code(dev, EV_ABS, ABS_RY, abs_ry);
    libevdev_enable_event_code(dev, EV_ABS, ABS_Z, &abs_z);
    libevdev_enable_event_code(dev, EV_ABS, ABS_RZ, abs_rz);
    libevdev_enable_event_code(dev, EV_ABS, ABS_HAT0X, &abs_hat0x);
    libevdev_enable_event_code(dev, EV_ABS, ABS_HAT0Y, abs_hat0y);

    libevdev_enable_event_type(dev, EV_FF);
    libevdev_enable_event_code(dev, EV_FF, FF_RUMBLE, NULL);
    libevdev_enable_event_code(dev, EV_FF, FF_PERIODIC, NULL);
    libevdev_enable_event_code(dev, EV_FF, FF_SQUARE, NULL);
    libevdev_enable_event_code(dev, EV_FF, FF_TRIANGLE, NULL);
    libevdev_enable_event_code(dev, EV_FF, FF_SINE, NULL);
    libevdev_enable_event_code(dev, EV_FF, FF_GAIN, NULL);

    libevdev_uinput_create_from_device(dev, LIBEVDEV_UINPUT_OPEN_MANAGED,
                                       uidev);

    libevdev_free(dev);
}
