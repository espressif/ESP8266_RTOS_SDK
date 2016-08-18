@echo off

Rem ******NOTICE******
Rem MUST set SDK_PATH & BIN_PATH firstly!!!
Rem example:
Rem set SDK_PATH=/c/esp_iot_sdk_freertos
Rem set BIN_PATH=/c/esp8266_bin

set SDK_PATH=""
set BIN_PATH=""

echo gen_misc.bat version 20150911
echo .

if not %SDK_PATH% == "" (
    echo SDK_PATH: %SDK_PATH%
) else (
    echo ERROR: Please set SDK_PATH in gen_misc.bat firstly, exit!!!
    goto end
)

if not %BIN_PATH% == "" (
    echo BIN_PATH: %BIN_PATH%
) else (
    echo ERROR: Please set BIN_PATH in gen_misc.bat firstly, exit!!!
    goto end
)

echo .
echo Please check SDK_PATH/BIN_PATH, enter (Y/y) to continue:
set input=default
set /p input=

if not %input% == Y (
    if not %input% == y (
        goto end
    )
)

echo .
echo Please follow below steps(1-5) to generate specific bin(s):
echo STEP 1: use boot_v1.2+ by default
set boot=new

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

echo STEP 5: choose flash size and map
echo     0= 512KB( 256KB+ 256KB)
echo     2=1024KB( 512KB+ 512KB)
echo     3=2048KB( 512KB+ 512KB)
echo     4=4096KB( 512KB+ 512KB)
echo     5=2048KB(1024KB+1024KB)
echo     6=4096KB(1024KB+1024KB)
set input=default
set /p input=enter (0/1/2/3/4/5/6, default 0):

if %input% equ 2 (
  set spi_size_map=2
  echo spi size: 1024KB
  echo spi ota map: 512KB + 512KB
) else (
  if %input% equ 3 (
    set spi_size_map=3
    echo spi size: 2048KB
    echo spi ota map: 512KB + 512KB
  ) else (
    if %input% equ 4 (
      set spi_size_map=4
      echo spi size: 4096KB
      echo spi ota map: 512KB + 512KB
    ) else (
      if %input% equ 5 (
        set spi_size_map=5
        echo spi size: 2048KB
        echo spi ota map: 1024KB + 1024KB
      ) else (
        if %input% equ 6 (
          set spi_size_map=6
          echo spi size: 4096KB
          echo spi ota map: 1024KB + 1024KB
        ) else (
          set spi_size_map=0
          echo spi size: 512KB
          echo spi ota map: 256KB + 256KB
        )
      )
    )
  )
)

echo.
echo start...
echo.

make clean

make COMPILE=xcc BOOT=%boot% APP=%app% SPI_SPEED=%spi_speed% SPI_MODE=%spi_mode% SPI_SIZE_MAP=%spi_size_map%

:end