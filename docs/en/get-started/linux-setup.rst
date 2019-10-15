*************************************
Standard Setup of Toolchain for Linux
*************************************

Install Prerequisites
=====================

To compile with ESP8266_RTOS_SDK you need to get the following packages:

- CentOS 7::

    sudo yum install gcc git wget make ncurses-devel flex bison gperf python pyserial

- Ubuntu and Debian::

    sudo apt-get install gcc git wget make libncurses-dev flex bison gperf python python-serial

- Arch::

    sudo pacman -S --needed gcc git make ncurses flex bison gperf python2-pyserial


Toolchain Setup
===============

ESP8266 toolchain for Linux is available for download from Espressif website:

- for 64-bit Linux:

  https://dl.espressif.com/dl/xtensa-lx106-elf-linux64-1.22.0-100-ge567ec7-5.2.0.tar.gz

- for 32-bit Linux:

  https://dl.espressif.com/dl/xtensa-lx106-elf-linux32-1.22.0-100-ge567ec7-5.2.0.tar.gz

1.  Download this file, then extract it in ``~/esp`` directory::

        mkdir -p ~/esp
        cd ~/esp
        tar -xzf ~/Downloads/xtensa-lx106-elf-linux64-1.22.0-100-ge567ec7-5.2.0.tar.gz

.. _setup-linux-toolchain-add-it-to-path:

2.  The toolchain will be extracted into ``~/esp/xtensa-lx106-elf/`` directory.

    To use it, you will need to update your ``PATH`` environment variable in ``~/.profile`` file. To make ``xtensa-lx106-elf`` available for all terminal sessions, add the following line to your ``~/.profile`` file::

        export PATH="$PATH:$HOME/esp/xtensa-lx106-elf/bin"

    Alternatively, you may create an alias for the above command. This way you can get the toolchain only when you need it. To do this, add different line to your ``~/.profile`` file::

        alias get_lx106='export PATH="$PATH:$HOME/esp/xtensa-lx106-elf/bin"'

    Then when you need the toolchain you can type ``get_lx106`` on the command line and the toolchain will be added to your ``PATH``.

    .. note::

        If you have ``/bin/bash`` set as login shell, and both ``.bash_profile`` and ``.profile`` exist, then update ``.bash_profile`` instead.

3.  Log off and log in back to make the ``.profile`` changes effective. Run the following command to verify if ``PATH`` is correctly set::

        printenv PATH

    You are looking for similar result containing toolchain's path at the end of displayed string::

        $ printenv PATH
        /home/user-name/bin:/home/user-name/.local/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/snap/bin:/home/user-name/esp/xtense-lx106-elf/bin

    Instead of ``/home/user-name`` there should be a home path specific to your installation.


Permission issues /dev/ttyUSB0
------------------------------

With some Linux distributions you may get the ``Failed to open port /dev/ttyUSB0`` error message when flashing the ESP8266.

If this happens you may need to add your current user to the correct group (commonly "dialout") which has the appropriate permissions::

    sudo usermod -a -G dialout $USER

In addition, you can also use "sudo chmod" to set permissions on the "/dev/ttyUSB0" file before running the make command to resolve::

    sudo chmod -R 777 /dev/ttyUSB0


Next Steps
==========

To carry on with development environment setup, proceed to section :ref:`get-started-get-esp-idf`.


Related Documents
=================


.. _AUR: https://wiki.archlinux.org/index.php/Arch_User_Repository
