// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __ESP_ATTR_H__
#define __ESP_ATTR_H__

//Normally, the linker script will put all code and rodata in flash.
//These macros can be used to redirect particular functions/variables
//to other memory regions.

// Make compatible to old SDK
#define ICACHE_FLASH_ATTR

// Make compatible to old SDK
#define ICACHE_RODATA_ATTR

// Forces code into IRAM instead of flash.
#define IRAM_ATTR _SECTION_ATTR_IMPL(".iram1", __COUNTER__)

// Forces data into DRAM instead of flash
#define DRAM_ATTR _SECTION_ATTR_IMPL(".dram0", __COUNTER__)

// Forces data to be 4 bytes aligned
#define WORD_ALIGNED_ATTR __attribute__((aligned(4)))

// Forces a string into DRAM instead of flash
// Use as ets_printf(DRAM_STR("Hello world!\n"));
#define DRAM_STR(str) (__extension__({static const DRAM_ATTR char __c[] = (str); (const char *)&__c;}))

// Forces data into RTC memory.
// Any variable marked with this attribute will keep its value
// during a deep sleep / wake cycle.
#define RTC_DATA_ATTR _SECTION_ATTR_IMPL(".rtc.data", __COUNTER__)

// Forces read-only data into RTC memory.
#define RTC_RODATA_ATTR _SECTION_ATTR_IMPL(".rtc.rodata", __COUNTER__)

// Forces to put some user defined data in the binary file header, the offset is 0x10.
#define USER_DATA_ATTR _SECTION_ATTR_IMPL(".user.data", __COUNTER__)

// Implementation for a unique custom section
//
// This prevents gcc producing "x causes a section type conflict with y"
// errors if two variables in the same source file have different linkage (maybe const & non-const) but are placed in the same custom section
//
// Using unique sections also means --gc-sections can remove unused
// data with a custom section type set
#define _SECTION_ATTR_IMPL(SECTION, COUNTER) __attribute__((section(SECTION "." _COUNTER_STRINGIFY(COUNTER))))

#define _COUNTER_STRINGIFY(COUNTER) #COUNTER

#endif /* __ESP_ATTR_H__ */
