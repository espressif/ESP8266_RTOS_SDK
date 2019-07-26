/* ESP8266 Linker Script Memory Layout

   This file describes the memory layout (memory blocks).

   esp8266.project.ld contains output sections to link compiler output
   into these memory blocks.

   ***

   This linker script is passed through the C preprocessor to include
   configuration options.

   Please use preprocessor features sparingly! Restrict
   to simple macros with numeric values, and/or #if/#endif blocks.
*/
#include "sdkconfig.h"


MEMORY
{
  /* All these values assume the flash cache is on, and have the blocks this uses subtracted from the length
  of the various regions. */

  /* IRAM for cpu. The length is due to the cache mode which is able to be set half or full mode. */
  iram0_0_seg (RX) :                 org = 0x40100000, len = CONFIG_SOC_IRAM_SIZE

  /* Even though the segment name is iram, it is actually mapped to flash and mapped constant data */
  iram0_2_seg (RX) :                 org = 0x40200010 + APP_OFFSET,
                                     len = APP_SIZE - 0x10

  /*
    (0x18 offset above is a convenience for the app binary image generation. The .bin file which is flashed
    to the chip has a 0x10 byte file header. Setting this offset makes it simple to meet the flash cache.)
  */


  /* Length of this section is 96KB */
  dram0_0_seg (RW) :                 org = 0x3FFE8000, len = 0x18000

  /* (See iram0_2_seg for meaning of 0x10 offset in the above.) */

  /* RTC memory. Persists over deep sleep */
  rtc_data_seg(RW) :                 org = 0x60001200, len = 0x200
}
