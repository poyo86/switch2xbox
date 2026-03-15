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
#include <pthread.h>
#include <sys/inotify.h>
#include <libevdev-1.0/libevdev/libevdev.h>

void *to_xbox(void *dev) {
    libevdev_grab(dev, LIBEVDEV_GRAB);

    // TODO: Create a virtual Xbox Controller and redirect the inputs there

    int fd = libevdev_get_fd(dev);

    // Free resources after the controller is disconnected
    libevdev_free(dev);
    if (close(fd) == -1) {
        perror("[WARNING]: Could not close file descriptor");
    }
    return NULL;
}

/*
 * Return codes:
 * 0 - the device is a supported controller
 * 1 - the device is not a supported controller, but no error was found
 * -1 - something went wrong in the function
 */
int handle_controller(int fd) {
    struct libevdev *dev;

    /*
     * TODO: Investigate where this could fail
     * Should the entire process exit?
     */
    if (libevdev_new_from_fd(fd, &dev) < 0) {
        perror("[ERROR]: Failed to open device");
        return -1;
    }

    const int id_vendor = libevdev_get_id_vendor(dev);
    const int id_product = libevdev_get_id_product(dev);
    if (id_vendor == NINTENDO_VENDOR_ID) {
        if (id_product == NINTENDO_PRO_CONTROLLER &&
            libevdev_has_event_code(dev, EV_KEY, BTN_SOUTH))
        {
            /* fprintf(stderr, "%s found at %s\n", libevdev_get_name(dev),
             *        get path from fd);
             * I don't like printing the same line from different parts of the
             * program */
            pthread_t controller_thread;

            int rc = pthread_create(&controller_thread, NULL, to_xbox, dev);

            if (rc != 0) {
                fprintf(stderr, "Failed to create controller thread: %s\n",
                        strerror(rc));

                libevdev_free(dev);
                return -1;
            } else {
                // NOTE: This will be removed in the future
                fprintf(stderr, "%s found ", libevdev_get_name(dev));
            }

            return 0;
        }
    }

    libevdev_free(dev);
    return 1;
}

int main() {
    fprintf(stderr, "switch2xbox " VERSION "\n"
            "Starting daemon...\n"
            "Looking for Nintendo Switch Controllers...\n");

    DIR *dir = opendir(DEVINPUT_DIR);
    struct dirent *entry;

    if (dir == NULL) {
        fprintf(stderr, "[ERROR]: Cannot open directory %s: ", DEVINPUT_DIR);
        perror(NULL);
        fprintf(stderr, "Exiting...\n");
        return 1;
    }

    while ((entry = readdir(dir)) != NULL) {
        int fd;
        char eventdevice[19];

        strcpy(eventdevice, DEVINPUT_DIR);

        // Only event* devices should be checked
        if (strncmp(entry->d_name, "ev", 2) == 0) {
            strcat(eventdevice, entry->d_name);
            fd = open(eventdevice, O_RDWR|O_NONBLOCK);

            if (fd != -1) {
                int rc = handle_controller(fd);

                if (rc != 0) {
                    if (close(fd) == -1) {
                        perror("[WARNING]: Could not close file descriptor");
                    }

                    if (rc == -1) {
                        return 1;
                    }
                } else {
                    /* Continuation of printed message of handle_controller()
                     * when a controller was found */
                    // NOTE: This will be removed in the future
                    fprintf(stderr, "at %s\n", eventdevice);
                }
            }
        }
    }

    if (closedir(dir) == -1) {
        fprintf(stderr, "[WARNING]: Could not close directory %s: ",
                DEVINPUT_DIR);
        perror(NULL);
    }

    int fd_inotify = inotify_init();
    if (fd_inotify == -1) {
        perror("[ERROR]: Failed to initialize inotify instance");
        return 1;
    }

    int wd = inotify_add_watch(fd_inotify, DEVINPUT_DIR,
                               IN_CREATE|IN_DELETE|IN_ATTRIB);
    if (wd == -1) {
        perror("[ERROR]: Failed to add new watch");
        close(fd_inotify);
        return 1;
    }

    char buf[4096];
    const struct inotify_event *event;
    ssize_t size;

    for (;;) {
        size = read(fd_inotify, buf, sizeof(buf));
        if (size == -1) {
            perror("[ERROR]: Failed to read inotify events\n");
            close(fd_inotify);
            return 1;
        }

        for (char *ptr = buf; ptr < buf + size;
            ptr += sizeof(struct inotify_event) + event->len)
        {
            event = (const struct inotify_event *) ptr;

            if (event->mask & IN_CREATE || event->mask & IN_ATTRIB) {
                /* TODO: Handle device creation or attribute change.
                 * Multiple events will be generated on the same device,
                 * however the same device must be handled only once */
            } else if (event->mask & IN_DELETE) {
                // TODO: Handle device removal
            }
        }
    }

    close(fd_inotify);

    return 0;
}
