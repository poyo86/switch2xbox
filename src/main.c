/*
 *  Make your Nintendo Switch Controller appear as an Xbox Controller
 *  Copyright (C) 2026  Daniel Delgado
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#define VERSION "v0.0.1"

#define DEVINPUT_DIR "/dev/input/"

#define NINTENDO_VENDOR_ID 0x57e
#define NINTENDO_PRO_CONTROLLER 0x2009

#include <stdio.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/inotify.h>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <libevdev-1.0/libevdev/libevdev-uinput.h>

#include "controller_management.h"

// Linked list to store all the connected controllers
controller *controller_list = NULL;

pthread_mutex_t lock;

void *to_xbox(void *dev) {
    libevdev_grab(dev, LIBEVDEV_GRAB);

    struct libevdev_uinput *uidev = NULL;
    create_xbox_controller(&uidev);

    if (uidev == NULL) {
        fprintf(stderr, "[ERROR]: Failed to create Xbox virtual controller\n");
        exit(EXIT_FAILURE);
    }

    int fd = libevdev_get_fd(dev);

    // Free resources after the controller is disconnected
    libevdev_uinput_destroy(uidev);
    libevdev_free(dev);
    if (close(fd) == -1) {
        perror("[WARNING]: Could not close file descriptor");
    }
    remove_controller(pthread_self());
    return NULL;
}

/*
 * Return codes:
 * 0 - the device is a supported controller
 * 1 - the device is not a supported controller
 */
int handle_controller(int fd, const char file_name[]) {
    struct libevdev *dev;

    if (libevdev_new_from_fd(fd, &dev) < 0) {
        perror("[ERROR]: Failed to open device");
        exit(EXIT_FAILURE);
    }

    const int id_vendor = libevdev_get_id_vendor(dev);
    const int id_product = libevdev_get_id_product(dev);
    if (id_vendor == NINTENDO_VENDOR_ID) {
        if (id_product == NINTENDO_PRO_CONTROLLER &&
            libevdev_has_event_code(dev, EV_KEY, BTN_GAMEPAD))
        {
            fprintf(stderr, "%s found at " DEVINPUT_DIR "%s\n",
                    libevdev_get_name(dev), file_name);

            pthread_t controller_thread;
            pthread_attr_t attr;

            pthread_attr_init(&attr);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

            int rc = pthread_create(&controller_thread, &attr, to_xbox, dev);

            pthread_attr_destroy(&attr);

            if (rc == 0) {
                add_controller(file_name, controller_thread);
            } else {
                fprintf(stderr, "Failed to create controller thread: %s\n",
                        strerror(rc));

                libevdev_free(dev);
                exit(EXIT_FAILURE);
            }

            return 0;
        }
    }

    libevdev_free(dev);
    return 1;
}

void read_device(const char *file_name) {
    int fd;
    char device_path[19] = DEVINPUT_DIR;

    // Only event* devices should be checked
    if (strncmp(file_name, "ev", 2) == 0) {
        strlcat(device_path, file_name, sizeof(device_path));
        fd = open(device_path, O_RDWR|O_NONBLOCK);

        if (fd != -1) {
            int rc = handle_controller(fd, file_name);

            if (rc == 1) {
                if (close(fd) == -1) {
                    perror("[WARNING]: Could not close file descriptor");
                }
            }
        }
    }
}

int main() {
    pthread_mutex_init(&lock, NULL);

    fprintf(stderr, "switch2xbox " VERSION "\n"
            "Starting daemon...\n"
            "Looking for Nintendo Switch Controllers...\n");

    DIR *dir = opendir(DEVINPUT_DIR);
    struct dirent *entry;

    if (dir == NULL) {
        fprintf(stderr, "[ERROR]: Cannot open directory %s: ", DEVINPUT_DIR);
        perror(NULL);
        fprintf(stderr, "Exiting...\n");
        return EXIT_FAILURE;
    }

    while ((entry = readdir(dir)) != NULL) {
        read_device(entry->d_name);
    }

    if (closedir(dir) == -1) {
        fprintf(stderr, "[WARNING]: Could not close directory %s: ",
                DEVINPUT_DIR);
        perror(NULL);
    }

    int fd_inotify = inotify_init();
    if (fd_inotify == -1) {
        perror("[ERROR]: Failed to initialize inotify instance");
        return EXIT_FAILURE;
    }

    int wd = inotify_add_watch(fd_inotify, DEVINPUT_DIR,
                               IN_CREATE|IN_DELETE|IN_ATTRIB);
    if (wd == -1) {
        perror("[ERROR]: Failed to add new watch");
        close(fd_inotify);
        return EXIT_FAILURE;
    }

    char buf[4096];
    const struct inotify_event *event;
    ssize_t size;

    for (;;) {
        size = read(fd_inotify, buf, sizeof(buf));
        if (size == -1) {
            perror("[ERROR]: Failed to read inotify events\n");
            close(fd_inotify);
            return EXIT_FAILURE;
        }

        for (char *ptr = buf; ptr < buf + size;
            ptr += sizeof(struct inotify_event) + event->len)
        {
            event = (const struct inotify_event *) ptr;

            if (event->mask & IN_CREATE) {
                /* If IN_CREATE event is registered, it means the device didn't
                 * exist before, so no need to check the controller_list */
                read_device(event->name);
            } else if (event->mask & IN_ATTRIB) {
                /* Multiple events will be generated on the same device.
                 * For safety and sanity, it must be assumed that the user will
                 * have read access to the /dev/input/eventX file the entire
                 * time, which means multiple threads for the same controller
                 * would be created. controller_list exists to prevent this */
                pthread_mutex_lock(&lock);
                controller *ptr = controller_list;
                while (ptr != NULL) {
                    if (strncmp(ptr->file_name, event->name,
                                sizeof(ptr->file_name)) == 0)
                    {
                        pthread_mutex_unlock(&lock);
                        goto ignore_controller;
                    }
                    ptr = ptr->next;
                }
                pthread_mutex_unlock(&lock);
                read_device(event->name);
        ignore_controller:
                ;
            } else if (event->mask & IN_DELETE) {
                /* NOTE: This section may not be needed, as the device removal
                 * can probably be detected in the to_xbox() function */
            }
        }
    }

    close(fd_inotify);

    return 0;
}
