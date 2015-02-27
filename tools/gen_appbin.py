#!/usr/bin/python
#
#  Copyright (c) 2010 Espressif System
#
#     grab user_start() address and pass it to genflashbin program
#

import string
import sys
import os
import re
import subprocess

if len(sys.argv) != 3:
    print 'Usage: gen_appbin.py eagle.app.out version'
    sys.exit(0)

elf_file = sys.argv[1]
ver = sys.argv[2]
#print elf_file

try:
    cross_compile_prefix = os.environ['CROSS_COMPILE']
except KeyError:
    if subprocess.call("xtensa-lx106-elf-nm --version", shell=True) == 0:
        cross_compile_prefix = 'xtensa-lx106-elf-'
    else:
        cross_compile_prefix = 'xt-'

cmd = '%snm -g ' %(cross_compile_prefix) + elf_file + ' > eagle.app.sym'
#print cmd
os.system(cmd)

fp = file('./eagle.app.sym')
if fp is None:
    print "open sym file error\n"
    exit

lines = fp.readlines()

fp.close()

entry_addr = None
p = re.compile('(\w*)(\sT\s)(call_user_start)$')
for line in lines:
    m = p.search(line)
    if m != None:
        entry_addr = m.group(1)
        #entry_addr = int(entry_addr, 16)
        print entry_addr

if entry_addr is None:
    print 'no entry point!!'
    exit

data_start_addr = '0'
p = re.compile('(\w*)(\sA\s)(_data_start)$')
for line in lines:
    m = p.search(line)
    if m != None:
        data_start_addr = m.group(1)
        print data_start_addr

rodata_start_addr = '0'
p = re.compile('(\w*)(\sA\s)(_rodata_start)$')
for line in lines:
    m = p.search(line)
    if m != None:
        rodata_start_addr = m.group(1)
        print rodata_start_addr

cmd = 'esptool.py make_image -f eagle.app.%s.text.bin '%(ver)+' -a 0x' + entry_addr + ' -f eagle.app.%s.data.bin'%(ver) + ' -a 0x' + data_start_addr +' -f eagle.app.%s.rodata.bin'%(ver)+ ' -a 0x' + rodata_start_addr + ' eagle.app.flash.bin'

print cmd
os.system(cmd)

cmd = 'mv eagle.app.flash.bin eagle.app.%s.flash.bin'%(ver)

print cmd
os.system(cmd)
