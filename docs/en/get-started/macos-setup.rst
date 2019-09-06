**************************************
Standard Setup of Toolchain for Mac OS
**************************************

Install Prerequisites
=====================

- install pip::

    sudo easy_install pip

- install pyserial::

    sudo pip install pyserial


Toolchain Setup
===============

ESP8266 toolchain for macOS is available for download from Espressif website:

https://dl.espressif.com/dl/xtensa-lx106-elf-macos-1.22.0-100-ge567ec7-5.2.0.tar.gz

Download this file, then extract it in ``~/esp`` directory::

    mkdir -p ~/esp
    cd ~/esp
    tar -xzf ~/Downloads/xtensa-lx106-elf-macos-1.22.0-100-ge567ec7-5.2.0.tar.gz

.. _setup-macos-toolchain-add-it-to-path:

The toolchain will be extracted into ``~/esp/xtensa-lx106-elf/`` directory.

To use it, you will need to update your ``PATH`` environment variable in ``~/.profile`` file. To make ``xtensa-lx106-elf`` available for all terminal sessions, add the following line to your ``~/.profile`` file::

    export PATH=$PATH:$HOME/esp/xtensa-lx106-elf/bin

Alternatively, you may create an alias for the above command. This way you can get the toolchain only when you need it. To do this, add different line to your ``~/.profile`` file::

    alias get_lx106="export PATH=$PATH:$HOME/esp/xtensa-lx106-elf/bin"

Then when you need the toolchain you can type ``get_lx106`` on the command line and the toolchain will be added to your ``PATH``.


Next Steps
==========

To carry on with development environment setup, proceed to section :ref:`get-started-get-esp-idf`.
