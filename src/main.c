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

#include <stdio.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <libevdev-1.0/libevdev/libevdev.h>

/*
 * Return codes:
 * 0 - the device is a supported controller
 * 1 - the device is not a supported controller, but no error was found
 * -1 - something went wrong when processing the device
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

    if (libevdev_get_id_vendor(dev) == 0x57e) {
        if (libevdev_get_id_product(dev) == 0x2009 &&
            libevdev_has_event_code(dev, EV_KEY, BTN_SOUTH))
        {
            fprintf(stderr, "%s found ", libevdev_get_name(dev));
            /* TODO: Create new thread to handle the found controller.
             * Don't forget to close the file descriptor after the controller
             * is disconnected */

            libevdev_free(dev);
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

    char devinput[] = "/dev/input/";
    DIR *dir = opendir(devinput);
    struct dirent *entry;

    if (dir == NULL) {
        fprintf(stderr, "[ERROR]: Cannot open directory %s: ", devinput);
        perror(NULL);
        fprintf(stderr, "Exiting...\n");
        return 1;
    }

    while ((entry = readdir(dir)) != NULL) {
        int fd;
        char eventdevice[19];

        strcpy(eventdevice, devinput);

        if (entry->d_type == DT_CHR) {
            strcat(eventdevice, entry->d_name);
            fd = open(eventdevice, O_RDWR|O_NONBLOCK);

            if (fd != -1) {
                int rc;

                // Only event* devices should be checked
                if (strncmp(entry->d_name, "ev", 2) == 0) {
                    rc = handle_controller(fd);
                }

                if (rc != 0) {
                    if (close(fd) == -1) {
                        perror("[WARNING]: Could not close file descriptor");
                    }
                } else {
                    /* Continuation of printed message of handle_controller()
                     * when a controller was found */
                    fprintf(stderr, "at %s\n", eventdevice);
                }
            }
        }
    }

    if (closedir(dir) == -1) {
        fprintf(stderr, "[WARNING]: Could not close directory %s: ", devinput);
        perror(NULL);
    }

    // int fd = inotify_init();
    // int wd = inotify_add_watch(fd, devinput, IN_CREATE|IN_DELETE);

    // TODO: Wait for new devices to be connected

    return 0;
}
