/* user2.bin @ 0x41000 */

/* Flash Map (256KB + 256KB), support 512KB SPI Flash */
/* |..|........................|.....|.....|..|........................|.....|....|                       */
/* ^  ^                        ^     ^     ^  ^                        ^     ^                            */
/* |_boot start(0x0000)        |     |     |_pad start(0x40000)        |     |                            */
/*    |_user1 start(0x1000)    |_user1 end    |_user2 start(0x41000)   |_user2 end                        */
/*                                   |_system param symmetric area(0x3b000)  |_system param area(0x7b000) */

/* NOTICE: */ 
/* 1. You can change irom0 len, but MUST make sure user2 end not overlap system param area. */
/* 2. Space between user2 end and system param area can be used as user param area.         */
/* 3. Don't change any other seg.                                                           */

MEMORY
{
  dport0_0_seg :                        org = 0x3FF00000, len = 0x10
  dram0_0_seg :                         org = 0x3FFE8000, len = 0x18000
  iram1_0_seg :                         org = 0x40100000, len = 0x8000
  irom0_0_seg :                         org = 0x40241010, len = 0x2B000
}

INCLUDE "../ld/eagle.app.v6.common.ld"
