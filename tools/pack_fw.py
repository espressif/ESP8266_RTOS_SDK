#!/usr/bin/env python2
#
# Copyright 2018-2019 Espressif Systems (Shanghai) PTE LTD
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import argparse
import inspect
import sys
import binascii
import struct
import logging

__version__ = "1.0.2"

FLASH_SECTOR_SIZE = 0x1000

PYTHON2 = sys.version_info[0] < 3

def esp8266_crc32(data):
    """
    CRC32 algorithm used by 8266 SDK bootloader (and gen_appbin.py).
    """
    crc = binascii.crc32(data, 0) & 0xFFFFFFFF
    if crc & 0x80000000:
        return crc ^ 0xFFFFFFFF
    else:
        return crc + 1

def version(args):
    print(__version__)

# python pack_fw.py addr1 bin1 addr2 bin2 ...... 
# The address must increase.

class proc_addr_file(argparse.Action):
    """ Custom parser class for the address/filename pairs passed as arguments """
    def __init__(self, option_strings, dest, nargs='+', **kwargs):
        super(proc_addr_file, self).__init__(option_strings, dest, nargs, **kwargs)

    def __call__(self, parser, namespace, values, option_string=None):
        pairs = []
        for i in range(0, len(values) ,2):
            try:
                address = int(values[i], 0)
                if address == 0:
                    address = 0x1000
            except ValueError:
                raise argparse.ArgumentError(self, 'Address "%s" must be a number' % values[i])
            
            # ota initial data is not need
            if 'ota_data_initial.bin' not in values[i + 1]:
                try:
                    argfile = open(values[i + 1], 'rb')
                except IOError as e:
                    raise argparse.ArgumentError(self, e)
                pairs.append((address, argfile))

        end = 0
        pairs = sorted(pairs)
        for address, argfile in pairs:
            argfile.seek(0,2)  # seek to end
            size = argfile.tell()
            argfile.seek(0)
            sector_start = address & ~(FLASH_SECTOR_SIZE - 1)
            sector_end = ((address + size + FLASH_SECTOR_SIZE - 1) & ~(FLASH_SECTOR_SIZE - 1)) - 1
            if sector_start < end:
                message = 'Detected overlap at address: 0x%x for file: %s' % (address, argfile.name)
                raise argparse.ArgumentError(self, message)
            end = sector_end
        setattr(namespace, self.dest, pairs)

def pack3(args):
    fw_data = ''

    try:
        output_file = open(args.output, "w+")
    except IOError as e:
        print(e)

    end_addr = None
    prev_addr = 0
    prec_file = ''
    app_offset = 0
    for address, argfile in args.addr_filename:
        if end_addr is not None and address > end_addr:
            data = (address - end_addr) * ['ff']
            filled = binascii.a2b_hex(''.join(data))
            fw_data += filled

        try:
            argfile.seek(0, 0)
            data = argfile.read()
            fw_data += data

            argfile.seek(0, 2)
            prev_addr = address
            prec_file = argfile.name
            end_addr = address + argfile.tell()
            if app_offset is not 0:
                raise Exception('Partition %s can be put behind %s'%(argfile.name, args.app))
            else:
                if args.app in argfile.name:
                    app_offset = address - 0x1000
        except IOError as e:
            raise e

    if app_offset is 0:
        raise Exception('Failed to find application binary %s in all arguments'%args.app)

    crc32 = esp8266_crc32(fw_data)
    fw_data += struct.pack('<I', crc32)

    try:
        output_file.write(fw_data)
        output_file.close()
    except IOError as e:
        raise e

def main():
    parser = argparse.ArgumentParser(description='pack_fw v%s - ESP8266 ROM Bootloader Utility' % __version__, prog='pack_fw')
 
    parser.add_argument(
        '--output', '-o',
        help='Output file name with full path',
        default=None)

    parser.add_argument(
        '--app', '-a',
        help='application binary file name',
        default=None)

    subparsers = parser.add_subparsers(
        dest='operation',
        help='Run pack_fw {command} -h for additional help')

    parser_pack_fw = subparsers.add_parser(
        'pack3',
        help='Pack the V3 firmware')
    parser_pack_fw.add_argument('addr_filename', metavar='<address> <filename>', help='Address followed by binary filename, separated by space',
        action=proc_addr_file)

    print('pack_fw.py v%s' % __version__)

    args = parser.parse_args()

    if args.operation is None:
        parser.print_help()
        sys.exit(1)

    operation_func = globals()[args.operation]

    if PYTHON2:
        # This function is depreciated in Python3
        operation_args = inspect.getargspec(operation_func).args
    else:
        operation_args = inspect.getfullargspec(operation_func).args

    operation_func(args)

class FatalError(RuntimeError):
    """
    Wrapper class for runtime errors that aren't caused by internal bugs, but by
    ESP8266 responses or input content.
    """
    def __init__(self, message):
        RuntimeError.__init__(self, message)

    @staticmethod
    def WithResult(message, result):
        """
        Return a fatal error object that appends the hex values of
        'result' as a string formatted argument.
        """
        message += " (result was %s)" % hexify(result)
        return FatalError(message)

def _main():
    try:
        main()
    except FatalError as e:
        print('\nA fatal error occurred: %s' % e)
        sys.exit(2)

if __name__ == '__main__':
    _main()
