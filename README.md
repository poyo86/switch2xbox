> [!NOTE]
> This program is in early development. See [Status](#status)

switch2xbox
===========

Make your Nintendo Switch Controller appear as an Xbox Controller, no
configuration needed.

It is designed to work on Linux systems, though there is a possibility that it
will work on FreeBSD and derivatives (not tested yet).

Status
------

Early development. Not functional yet. Right now the program just detects
Nintendo Switch Pro Controllers, but it doesn't do anything with them yet. The
intended functionality of the program is not yet implemented.

Compilation
-----------

Required libraries and tools:

* git
* gcc or clang
* make
* pkg-config
* libevdev

Clone the repository:

    git clone https://github.com/poyo86/switch2xbox.git

Then, simply run:

    make

Voila! You can now run the program as `./switch2xbox` :)

Recommended setup
-----------------

* uinput kernel module should be loaded

      modprobe uinput

* Don't add yourself to the input group, use udev rules instead
    * [game-devices-udev](https://codeberg.org/fabiscafe/game-devices-udev)
    * [steam-devices](https://github.com/ValveSoftware/steam-devices)
* Install and setup [joycond](https://github.com/DanielOgorchock/joycond)

Similar projects
----------------

* [sc-controller](https://github.com/C0rn3j/sc-controller)
* [xboxdrv](https://github.com/xboxdrv/xboxdrv)
* [MoltenGamepad](https://github.com/jgeumlek/MoltenGamepad)
* [antimicrox](https://github.com/AntiMicroX/antimicrox)

---

This program contains 100% human written code, and I plan to keep it that way.
