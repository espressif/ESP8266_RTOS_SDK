#!/bin/bash
xtensa-lx106-elf-objcopy --redefine-sym ets_printf=phy_printf libphy.a 
