/*
    FreeRTOS V7.5.2 - Copyright (C) 2013 Real Time Engineers Ltd.

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that has become a de facto standard.             *
     *                                                                       *
     *    Help yourself get started quickly and support the FreeRTOS         *
     *    project by purchasing a FreeRTOS tutorial book, reference          *
     *    manual, or both from: http://www.FreeRTOS.org/Documentation        *
     *                                                                       *
     *    Thank you!                                                         *
     *                                                                       *
    ***************************************************************************

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>!AND MODIFIED BY!<< the FreeRTOS exception.

    >>! NOTE: The modification to the GPL is included to allow you to distribute
    >>! a combined work that includes FreeRTOS without being obliged to provide
    >>! the source code for proprietary components outside of the FreeRTOS
    >>! kernel.

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available from the following
    link: http://www.freertos.org/a00114.html

    1 tab == 4 spaces!

    ***************************************************************************
     *                                                                       *
     *    Having a problem?  Start by reading the FAQ "My application does   *
     *    not run, what could be wrong?"                                     *
     *                                                                       *
     *    http://www.FreeRTOS.org/FAQHelp.html                               *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org - Documentation, books, training, latest versions,
    license and Real Time Engineers Ltd. contact details.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.OpenRTOS.com - Real Time Engineers ltd license FreeRTOS to High
    Integrity Systems to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

/*
 * A sample implementation of pvPortMalloc() and vPortFree() that combines 
 * (coalescences) adjacent memory blocks as they are freed, and in so doing 
 * limits memory fragmentation.
 *
 * See heap_1.c, heap_2.c and heap_3.c for alternative implementations, and the 
 * memory management pages of http://www.FreeRTOS.org for more information.
 */
#include <stdlib.h>
#include <string.h>

/* Defining MPU_WRAPPERS_INCLUDED_FROM_API_FILE prevents task.h from redefining
all the API functions to use the MPU wrappers.  That should only be done when
task.h is included from an application file. */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "espressif/esp8266/ets_sys.h"

#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#if 1
#define mem_printf(fmt, args...) ets_printf(fmt,## args)
#else 
#define mem_printf(fmt, args...)
#endif

#define configTOTAL_HEAP_SIZE			( ( size_t ) ( 0x40000000 - (uint32)&_heap_start ) )

/* Block sizes must not get too small. */
#define heapMINIMUM_BLOCK_SIZE	( ( size_t ) ( heapSTRUCT_SIZE * 2 ) )

/* Assumes 8bit bytes! */
#define heapBITS_PER_BYTE		( ( size_t ) 8 )

/* A few bytes might be lost to byte aligning the heap start address. */
#define heapADJUSTED_HEAP_SIZE	( configTOTAL_HEAP_SIZE - portBYTE_ALIGNMENT )

/* Define the linked list structure.  This is used to link free blocks in order
of their memory address. */
typedef struct A_BLOCK_LINK
{
	struct A_BLOCK_LINK *pxNextFreeBlock;	/*<< The next free block in the list. */
	size_t xBlockSize;						/*<< The size of the free block. */
#ifdef MEMLEAK_DEBUG
        const char *file;
        unsigned line;
#endif
} xBlockLink;

/*-----------------------------------------------------------*/

/*
 * Inserts a block of memory that is being freed into the correct position in 
 * the list of free memory blocks.  The block being freed will be merged with
 * the block in front it and/or the block behind it if the memory blocks are
 * adjacent to each other.
 */
static void prvInsertBlockIntoFreeList( xBlockLink *pxBlockToInsert );

/*
 * Called automatically to setup the required heap structures the first time
 * pvPortMalloc() is called.
 */
static void prvHeapInit( void );


extern char _heap_start;
/*-----------------------------------------------------------*/
/* Allocate the memory for the heap. */
//static unsigned char ucHeap[ configTOTAL_HEAP_SIZE ];
static unsigned char *ucHeap;
/* The size of the structure placed at the beginning of each allocated memory
block must by correctly byte aligned. */
static unsigned short heapSTRUCT_SIZE;//  = ( ( sizeof ( xBlockLink ) + ( portBYTE_ALIGNMENT - 1 ) ) & ~portBYTE_ALIGNMENT_MASK );

static unsigned short portBYTE_ALIGNMENT_MASK_v;
static unsigned short portBYTE_ALIGNMENT_v;

/* Create a couple of list links to mark the start and end of the list. */
static xBlockLink xStart, *pxEnd = NULL;

#ifdef MEMLEAK_DEBUG
//add by jjj, we Link the used blocks here
static xBlockLink yStart;
static size_t yFreeBytesRemaining;
#endif

/* Ensure the pxEnd pointer will end up on the correct byte alignment. */
//static const size_t xTotalHeapSize = ( ( size_t ) heapADJUSTED_HEAP_SIZE ) & ( ( size_t ) ~portBYTE_ALIGNMENT_MASK );
static size_t xTotalHeapSize;

/* Keeps track of the number of free bytes remaining, but says nothing about
fragmentation. */
//static size_t xFreeBytesRemaining = ( ( size_t ) heapADJUSTED_HEAP_SIZE ) & ( ( size_t ) ~portBYTE_ALIGNMENT_MASK );
static size_t xFreeBytesRemaining;

/* Gets set to the top bit of an size_t type.  When this bit in the xBlockSize 
member of an xBlockLink structure is set then the block belongs to the 
application.  When the bit is free the block is still part of the free heap
space. */
static size_t xBlockAllocatedBit = 0;

#ifdef MEMLEAK_DEBUG
static const char mem_debug_file[] ICACHE_RODATA_ATTR STORE_ATTR = "user_app";
#endif

/*-----------------------------------------------------------*/
bool ICACHE_FLASH_ATTR __attribute__((weak))
check_memleak_debug_enable()
{
    return 0;
}
#ifdef MEMLEAK_DEBUG 
#include "spi_flash.h"
//extern SpiFlashChip *flashchip;
extern SpiFlashChip flashchip;
void prvInsertBlockIntoUsedList(xBlockLink *pxBlockToInsert)
{
        xBlockLink *pxIterator;
        for( pxIterator = &yStart; pxIterator->pxNextFreeBlock < pxBlockToInsert && pxIterator->pxNextFreeBlock != NULL;pxIterator = pxIterator->pxNextFreeBlock)
        {
                /* Nothing to do here. */
        }
        pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock;
        pxIterator->pxNextFreeBlock = pxBlockToInsert;
        yFreeBytesRemaining += pxBlockToInsert->xBlockSize;
}

static const char *ICACHE_FLASH_ATTR
vGetFileName(char *file_name_out, const char *file_name_in)
{
	if (((uint32)file_name_in & 0x40200000) == 0x40200000) {
		uint16 str_len = ((strlen(file_name_in) - 1) / 4 + 1) * 4;

		if (str_len > 32) {
			str_len = 32;
		}

		memcpy(file_name_out, file_name_in, str_len);
		file_name_out[str_len] = 0;

		return file_name_out;
	}

	return file_name_in;
}

void pvShowMalloc()
{
	xBlockLink *pxIterator;
//ets_printf("sh0:%d,%d,",heapSTRUCT_SIZE,sizeof( xBlockLink ));
	if(heapSTRUCT_SIZE < sizeof( xBlockLink ))
		return;
	ETS_INTR_LOCK();
	Wait_SPI_Idle(&flashchip);
	Cache_Read_Enable_New();
//ets_printf("sh1,");
	os_printf("--------Show Malloc--------\n");
	for( pxIterator = &yStart; pxIterator->pxNextFreeBlock != NULL;pxIterator = pxIterator->pxNextFreeBlock) {
		char file_name[33];
		const char *file_name_printf;
//ets_printf("sh2,");
		file_name_printf = vGetFileName(file_name, pxIterator->pxNextFreeBlock->file);
		os_printf("F:%s\tL:%u\tmalloc %u\t@ %x\n", file_name_printf, pxIterator->pxNextFreeBlock->line, pxIterator->pxNextFreeBlock->xBlockSize, ( void * ) ( ( ( unsigned char * ) pxIterator->pxNextFreeBlock ) + heapSTRUCT_SIZE));
//ets_printf("sh3,");
//		ets_delay_us(2000);
        system_soft_wdt_feed();
	}
	os_printf("--------Free %d--------\n\n", xFreeBytesRemaining);

#if 0
	uint32 last_link = (uint32)yStart.pxNextFreeBlock;
	uint32 index = 0;
	os_printf("'*':used, '-'free, each %d bytes\n", portBYTE_ALIGNMENT_v);
	os_printf("%x:", last_link);
	for( pxIterator = &yStart; pxIterator->pxNextFreeBlock != NULL;pxIterator = pxIterator->pxNextFreeBlock) {
	    uint16 i;
	    for (i = 0; i < ((uint32)pxIterator->pxNextFreeBlock - last_link) / portBYTE_ALIGNMENT_v; i++) {
	        index++;
	        os_printf("-");
	        if (index % 64 == 0) {
	            os_printf("\n%x:", (uint32)yStart.pxNextFreeBlock + index * portBYTE_ALIGNMENT_v);
	        }
	    }
	    for (i = 0; i < pxIterator->pxNextFreeBlock->xBlockSize / portBYTE_ALIGNMENT_v; i++) {
	        index++;
	        os_printf("*");
	        if (index % 64 == 0) {
	            os_printf("\n%x:", (uint32)yStart.pxNextFreeBlock + index * portBYTE_ALIGNMENT_v);
            }
	    }
	    last_link = ((uint32)pxIterator->pxNextFreeBlock + pxIterator->pxNextFreeBlock->xBlockSize);
        system_soft_wdt_feed();
    }
	os_printf("\n\n");
#endif

//ets_printf("sh4\n");
	ETS_INTR_UNLOCK();
}

void system_show_malloc(void) __attribute__((alias("pvShowMalloc")));

int prvRemoveBlockFromUsedList(xBlockLink *pxBlockToRemove)
{
        xBlockLink *pxIterator;
        for( pxIterator = &yStart; pxIterator->pxNextFreeBlock != pxBlockToRemove && pxIterator->pxNextFreeBlock != NULL;pxIterator = pxIterator->pxNextFreeBlock)
        {
                /* Nothing to do here. */
        }
        if(pxIterator->pxNextFreeBlock != pxBlockToRemove){
            return -1;
        }
        pxIterator->pxNextFreeBlock = pxBlockToRemove->pxNextFreeBlock;
        yFreeBytesRemaining -= pxBlockToRemove->xBlockSize;
        return 0;
}
#endif

size_t xPortWantedSizeAlign(size_t xWantedSize)
{
	xWantedSize += heapSTRUCT_SIZE;

	/* Ensure that blocks are always aligned to the required number
	of bytes. */
	if( ( xWantedSize & portBYTE_ALIGNMENT_MASK_v ) != 0x00 )
	{
		/* Byte alignment required. */
		xWantedSize += ( portBYTE_ALIGNMENT_v - ( xWantedSize & portBYTE_ALIGNMENT_MASK_v ) );
	}

	return xWantedSize;
}


#ifndef MEMLEAK_DEBUG
void *pvPortMalloc( size_t xWantedSize )
#else
void *pvPortMalloc( size_t xWantedSize, const char * file, unsigned line)
#endif
{
xBlockLink *pxBlock, *pxPreviousBlock, *pxNewBlockLink;
void *pvReturn = NULL;

//    printf("%s %d %d\n", __func__, xWantedSize, xFreeBytesRemaining);

//	vTaskSuspendAll();
	ETS_INTR_LOCK();
	{
		/* If this is the first call to malloc then the heap will require
		initialisation to setup the list of free blocks. */
		if( pxEnd == NULL )
		{
			prvHeapInit();
		}

			if( xWantedSize > 0 )
			{
				xWantedSize = xPortWantedSizeAlign(xWantedSize);
			}

			if( ( xWantedSize > 0 ) && ( xWantedSize < xFreeBytesRemaining ) )
			{
				/* Traverse the list from the start	(lowest address) block until 
				one	of adequate size is found. */
				pxPreviousBlock = &xStart;
				pxBlock = xStart.pxNextFreeBlock;
				while( ( pxBlock->xBlockSize < xWantedSize ) && ( pxBlock->pxNextFreeBlock != NULL ) )
				{
					pxPreviousBlock = pxBlock;
					pxBlock = pxBlock->pxNextFreeBlock;
				}

				/* If the end marker was reached then a block of adequate size 
				was	not found. */
				if( pxBlock != pxEnd )
				{
					/* Return the memory space pointed to - jumping over the 
					xBlockLink structure at its start. */
					pvReturn = ( void * ) ( ( ( unsigned char * ) pxPreviousBlock->pxNextFreeBlock ) + heapSTRUCT_SIZE );

					/* This block is being returned for use so must be taken out 
					of the list of free blocks. */
					pxPreviousBlock->pxNextFreeBlock = pxBlock->pxNextFreeBlock;

					/* If the block is larger than required it can be split into 
					two. */
					if( ( pxBlock->xBlockSize - xWantedSize ) > heapMINIMUM_BLOCK_SIZE )
					{
						/* This block is to be split into two.  Create a new 
						block following the number of bytes requested. The void 
						cast is used to prevent byte alignment warnings from the 
						compiler. */
						pxNewBlockLink = ( void * ) ( ( ( unsigned char * ) pxBlock ) + xWantedSize );

						/* Calculate the sizes of two blocks split from the 
						single block. */
						pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
						pxBlock->xBlockSize = xWantedSize;

					//Insert the new block into the list of free blocks.
					//ETS_INTR_LOCK();
					prvInsertBlockIntoFreeList( ( pxNewBlockLink ) );
					//ETS_INTR_UNLOCK();
				}
#ifdef MEMLEAK_DEBUG
			        pxBlock->pxNextFreeBlock = NULL;
#endif
				xFreeBytesRemaining -= pxBlock->xBlockSize;
#ifdef MEMLEAK_DEBUG
					if(heapSTRUCT_SIZE >= sizeof( xBlockLink )){
						pxBlock->file = file;
						pxBlock->line = line;
					}
					//link the use block
					prvInsertBlockIntoUsedList(pxBlock);
#endif
			}
		}
	}
//	xTaskResumeAll();
	ETS_INTR_UNLOCK();

	#if( configUSE_MALLOC_FAILED_HOOK == 1 )
	{
		if( pvReturn == NULL )
		{
			//extern void vApplicationMallocFailedHook( void );
			//vApplicationMallocFailedHook();
			ets_printf("E:M %d\n", xWantedSize);
		}
	}
	#endif
//mem_printf("%s %x\n", __func__, pvReturn);
//    printf("%s %x %x\n", __func__, pvReturn, pxBlock);
	return pvReturn;
}

//void *malloc(size_t nbytes) __attribute__((alias("pvPortMalloc")));

/*-----------------------------------------------------------*/
#ifndef MEMLEAK_DEBUG
void vPortFree( void *pv )
#else
void vPortFree(void *pv, const char * file, unsigned line)
#endif
{
unsigned char *puc = ( unsigned char * ) pv;
xBlockLink *pxLink;

//    printf("%s\n", __func__);
	if( pv != NULL )
	{
		/* The memory being freed will have an xBlockLink structure immediately
		before it. */
		puc -= heapSTRUCT_SIZE;

		/* This casting is to keep the compiler from issuing warnings. */
		pxLink = ( void * ) puc;

//				vTaskSuspendAll();
				ETS_INTR_LOCK();
				{
#ifdef MEMLEAK_DEBUG
					if(prvRemoveBlockFromUsedList(pxLink) < 0){
						ets_printf("%x already freed\n", pv);
					}
					else
#endif
					{
						/* Add this block to the list of free blocks. */
						xFreeBytesRemaining += pxLink->xBlockSize;
						prvInsertBlockIntoFreeList( ( ( xBlockLink * ) pxLink ) );
					}
				}
//				xTaskResumeAll();
				ETS_INTR_UNLOCK();
	}

//	printf("%s %x %d\n", __func__, pv, xFreeBytesRemaining);
//mem_printf("%s %x %d\n", __func__, pv, xFreeBytesRemaining);
}

//void free(void *ptr) __attribute__((alias("vPortFree")));

#ifndef MEMLEAK_DEBUG
void *malloc(size_t nbytes) __attribute__((alias("pvPortMalloc")));
void free(void *ptr) __attribute__((alias("vPortFree")));

/*-----------------------------------------------------------*/

void *pvPortCalloc(size_t count, size_t size)
{
  void *p;

  /* allocate 'count' objects of size 'size' */
  p = pvPortMalloc(count * size);
  if (p) {
    /* zero the memory */
    memset(p, 0, count * size);
  }
  return p;
}

void *calloc(size_t count, size_t nbytes) __attribute__((alias("pvPortCalloc")));

/*-----------------------------------------------------------*/

void *pvPortZalloc(size_t size)
{
     return pvPortCalloc(1, size);	
}

void *zalloc(size_t nbytes) __attribute__((alias("pvPortZalloc")));

/*-----------------------------------------------------------*/

void *pvPortRealloc(void *mem, size_t newsize)
{
    if (newsize == 0) {
        vPortFree(mem);
        return NULL;
    }

    void *p;
    p = pvPortMalloc(newsize);
    if (p) {
        /* zero the memory */
        if (mem != NULL) {
            memcpy(p, mem, newsize);
            vPortFree(mem);
        }
    }
    return p;
}

void *realloc(void *ptr, size_t nbytes) __attribute__((alias("pvPortRealloc")));
/*-----------------------------------------------------------*/

#else

/*-----------------------------------------------------------*/
void *pvPortCalloc(size_t count, size_t size, const char * file, unsigned line)
{
  void *p;
//ets_printf("1,");
  /* allocate 'count' objects of size 'size' */
  p = pvPortMalloc(count * size, file, line);
//ets_printf("2,");
  if (p) {
    /* zero the memory */
    memset(p, 0, count * size);
  }
//ets_printf("3,");
  return p;
}
/*-----------------------------------------------------------*/

void *pvPortZalloc(size_t size, const char * file, unsigned line)
{
     return pvPortCalloc(1, size, file, line);
}
/*-----------------------------------------------------------*/

void *pvPortRealloc(void *mem, size_t newsize, const char *file, unsigned line)
{
    if (newsize == 0) {
        vPortFree(mem, file, line);
        return NULL;
    }

    void *p;
    p = pvPortMalloc(newsize, file, line);
    if (p) {
        /* zero the memory */
        if (mem != NULL) {
            memcpy(p, mem, newsize);
            vPortFree(mem, file, line);
        }
    }
    return p;
}
/*-----------------------------------------------------------*/
//For user
void *malloc(size_t nbytes)
{
//ets_printf("u_m\n");
	return pvPortMalloc( nbytes, mem_debug_file, 0);
}

void free(void *ptr)
{
//ets_printf("u_f\n");
	vPortFree(ptr, mem_debug_file, 0);
}

void *zalloc(size_t nbytes)
{
	return pvPortZalloc(nbytes, mem_debug_file, 0);
}

void *calloc(size_t count, size_t nbytes)
{
	return pvPortCalloc(count, nbytes, mem_debug_file, 0);
}

void *realloc(void *ptr, size_t nbytes)
{
	return pvPortRealloc(ptr, nbytes, mem_debug_file, 0);
}

/*
void *malloc(size_t nbytes) __attribute__((alias("malloc1")));
void free(void *ptr) __attribute__((alias("free1")));
void *calloc(size_t count, size_t nbytes) __attribute__((alias("zalloc1")));
void *zalloc(size_t nbytes) __attribute__((alias("calloc1")));
void *realloc(void *ptr, size_t nbytes) __attribute__((alias("realloc1")));
*/
#endif

size_t ICACHE_FLASH_ATTR
xPortGetFreeHeapSize( void )
{
	return xFreeBytesRemaining;
}
/*-----------------------------------------------------------*/

void ICACHE_FLASH_ATTR
vPortInitialiseBlocks( void )
{
	/* This just exists to keep the linker quiet. */
}
/*-----------------------------------------------------------*/

static void prvHeapInit( void )
{
xBlockLink *pxFirstFreeBlock;
unsigned char *pucHeapEnd, *pucAlignedHeap;

    if( check_memleak_debug_enable() ){
        portBYTE_ALIGNMENT_MASK_v = 0xf;
        portBYTE_ALIGNMENT_v = 16;
        heapSTRUCT_SIZE = ( (sizeof( xBlockLink ) + portBYTE_ALIGNMENT_MASK_v)& ~portBYTE_ALIGNMENT_MASK_v);
    } else {
        portBYTE_ALIGNMENT_MASK_v = 0x7;
        portBYTE_ALIGNMENT_v = 8;
        heapSTRUCT_SIZE = 8;
    }

    xFreeBytesRemaining = ( ( size_t ) heapADJUSTED_HEAP_SIZE ) & ( ( size_t ) ~portBYTE_ALIGNMENT_MASK_v );
    xTotalHeapSize = xFreeBytesRemaining ;
	ucHeap = &_heap_start;

	/* Ensure the heap starts on a correctly aligned boundary. */
	pucAlignedHeap = ( unsigned char * ) ( ( ( portPOINTER_SIZE_TYPE ) &ucHeap[ portBYTE_ALIGNMENT_MASK_v ] ) & ( ( portPOINTER_SIZE_TYPE ) ~portBYTE_ALIGNMENT_MASK_v ) );

	/* xStart is used to hold a pointer to the first item in the list of free
	blocks.  The void cast is used to prevent compiler warnings. */
	xStart.pxNextFreeBlock = ( void * ) pucAlignedHeap;
	xStart.xBlockSize = ( size_t ) 0;

	/* pxEnd is used to mark the end of the list of free blocks and is inserted
	at the end of the heap space. */
	pucHeapEnd = pucAlignedHeap + xTotalHeapSize;
	pucHeapEnd -= heapSTRUCT_SIZE;
	pxEnd = ( void * ) pucHeapEnd;
	//configASSERT( ( ( ( unsigned long ) pxEnd ) & ( ( unsigned long ) portBYTE_ALIGNMENT_MASK ) ) == 0UL );
	pxEnd->xBlockSize = 0;
	pxEnd->pxNextFreeBlock = NULL;

	/* To start with there is a single free block that is sized to take up the
	entire heap space, minus the space taken by pxEnd. */
	pxFirstFreeBlock = ( void * ) pucAlignedHeap;
	pxFirstFreeBlock->xBlockSize = xTotalHeapSize - heapSTRUCT_SIZE;
	pxFirstFreeBlock->pxNextFreeBlock = pxEnd;

	/* The heap now contains pxEnd. */
	xFreeBytesRemaining -= heapSTRUCT_SIZE;

#ifdef MEMLEAK_DEBUG
	//add by jjj
        yStart.pxNextFreeBlock = NULL;
	yStart.xBlockSize = ( size_t ) 0;
        yFreeBytesRemaining = 0;
#endif

}
/*-----------------------------------------------------------*/

static void prvInsertBlockIntoFreeList( xBlockLink *pxBlockToInsert )
{
xBlockLink *pxIterator;
unsigned char *puc;

	/* Iterate through the list until a block is found that has a higher address than the block being inserted. */
	for( pxIterator = &xStart; pxIterator->pxNextFreeBlock < pxBlockToInsert; pxIterator = pxIterator->pxNextFreeBlock )
	{
		/* Nothing to do here, just iterate to the right position. */
	}

	/* Do the block being inserted, and the block it is being inserted after
	make a contiguous block of memory? */	
	puc = ( unsigned char * ) pxIterator;
	if( ( puc + pxIterator->xBlockSize ) == ( unsigned char * ) pxBlockToInsert )
	{
		pxIterator->xBlockSize += pxBlockToInsert->xBlockSize;
		pxBlockToInsert = pxIterator;
	}

	/* Do the block being inserted, and the block it is being inserted before
	make a contiguous block of memory? */
	puc = ( unsigned char * ) pxBlockToInsert;
	if( ( puc + pxBlockToInsert->xBlockSize ) == ( unsigned char * ) pxIterator->pxNextFreeBlock )
	{
		if( pxIterator->pxNextFreeBlock != pxEnd )
		{
			/* Form one big block from the two blocks. */
			pxBlockToInsert->xBlockSize += pxIterator->pxNextFreeBlock->xBlockSize;
			pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock->pxNextFreeBlock;
		}
		else
		{
			pxBlockToInsert->pxNextFreeBlock = pxEnd;
		}
	}
	else
	{
		pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock;		
	}

	/* If the block being inserted plugged a gab, so was merged with the block
	before and the block after, then it's pxNextFreeBlock pointer will have
	already been set, and should not be set here as that would make it point
	to itself. */
	if( pxIterator != pxBlockToInsert )
	{
		pxIterator->pxNextFreeBlock = pxBlockToInsert;
	}
}

