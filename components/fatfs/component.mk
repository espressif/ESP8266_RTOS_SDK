COMPONENT_ADD_INCLUDEDIRS := diskio vfs src
COMPONENT_SRCDIRS := diskio vfs port/freertos src
COMPONENT_OBJEXCLUDE := src/diskio.o src/ffsystem.o diskio/diskio_sdmmc.o vfs/vfs_fat_sdmmc.o
