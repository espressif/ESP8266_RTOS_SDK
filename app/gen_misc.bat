@echo off

echo Please follow below steps(1-5) to generate specific bin(s):
echo STEP 1: choose boot version(0=boot_v1.1, 1=boot_v1.2+, 2=none)
set input=default
set /p input=enter(0/1/2, default 2):

if %input% equ 0 (
    set boot=old
) else (
if %input% equ 1 (
    set boot=new
) else (
    set boot=none
)
)

echo boot mode: %boot%
echo.

echo STEP 2: choose bin generate(0=eagle.flash.bin+eagle.irom0text.bin, 1=user1.bin, 2=user2.bin)
set input=default
set /p input=enter (0/1/2, default 0):

if %input% equ 1 (
    if %boot% equ none (
        set app=0
        echo choose no boot before
        echo generate bin: eagle.flash.bin+eagle.irom0text.bin
    ) else (
        set app=1
        echo generate bin: user1.bin
    )
) else (
if %input% equ 2 (
    if %boot% equ none (
        set app=0
        echo choose no boot before
        echo generate bin: eagle.flash.bin+eagle.irom0text.bin
    ) else (
        set app=2
        echo generate bin: user2.bin
    )
) else (
    if %boot% neq none (
        set boot=none
        echo ignore boot
    )
    set app=0
    echo generate bin: eagle.flash.bin+eagle.irom0text.bin
))

echo.

echo STEP 3: choose spi speed(0=20MHz, 1=26.7MHz, 2=40MHz, 3=80MHz)
set input=default
set /p input=enter (0/1/2/3, default 2):

if %input% equ 0 (
    set spi_speed=20
) else (
if %input% equ 1 (
    set spi_speed=26.7
) else (
if %input% equ 3 (
    set spi_speed=80
) else (
    set spi_speed=40
)))

echo spi speed: %spi_speed% MHz
echo.

echo STEP 4: choose spi mode(0=QIO, 1=QOUT, 2=DIO, 3=DOUT)
set input=default
set /p input=enter (0/1/2/3, default 0):

if %input% equ 1 (
    set spi_mode=QOUT
) else (
if %input% equ 2 (
    set spi_mode=DIO
) else (
if %input% equ 3 (
    set spi_mode=DOUT
) else (
    set spi_mode=QIO
)))

echo spi mode: %spi_mode%
echo.

echo STEP 5: choose spi size(0=256KB, 1=512KB, 2=1024KB, 3=2048KB, 4=4096KB)
set input=default
set /p input=enter (0/1/2/3/4, default 1):

if %input% equ 0 (
    set spi_size=256
) else (
if %input% equ 2 (
    set spi_size=1024
) else (
if %input% equ 3 (
    set spi_size=2048
) else (
if %input% equ 4 (
    set spi_size=4096
) else (
    set spi_size=512
))))

echo spi size: %spi_size% KB

touch user/user_main.c

echo.
echo start...
echo.

make BOOT=%boot% APP=%app% SPI_SPEED=%spi_speed% SPI_MODE=%spi_mode% SPI_SIZE=%spi_size%

