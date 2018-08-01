/*
    FreeRTOS V8.1.2 - Copyright (C) 2014 Real Time Engineers Ltd.
    All rights reserved

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

    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<

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
 * A sample implementation of pvPortMalloc() that allows the heap to be defined
 * across multiple non-contigous blocks and combines (coalescences) adjacent
 * memory blocks as they are freed.
 *
 * See heap_1.c, heap_2.c, heap_3.c and heap_4.c for alternative
 * implementations, and the memory management pages of http://www.FreeRTOS.org
 * for more information.
 *
 * Usage notes:
 *
 * vPortDefineHeapRegions() ***must*** be called before pvPortMalloc().
 * pvPortMalloc() will be called if any task objects (tasks, queues, event
 * groups, etc.) are created, therefore vPortDefineHeapRegions() ***must*** be
 * called before any other objects are defined.
 *
 * vPortDefineHeapRegions() takes a single parameter.  The parameter is an array
 * of HeapRegion_t structures.  HeapRegion_t is defined in portable.h as
 *
 * typedef struct HeapRegion
 * {
 *	uint8_t *pucStartAddress; << Start address of a block of memory that will be part of the heap.
 *	size_t xSizeInBytes;	  << Size of the block of memory.
 * } HeapRegion_t;
 *
 * The array is terminated using a NULL zero sized region definition, and the
 * memory regions defined in the array ***must*** appear in address order from
 * low address to high address.  So the following is a valid example of how
 * to use the function.
 *
 * HeapRegion_t xHeapRegions[] =
 * {
 * 	{ ( uint8_t * ) 0x80000000UL, 0x10000 }, << Defines a block of 0x10000 bytes starting at address 0x80000000
 * 	{ ( uint8_t * ) 0x90000000UL, 0xa0000 }, << Defines a block of 0xa0000 bytes starting at address of 0x90000000
 * 	{ NULL, 0 }                << Terminates the array.
 * };
 *
 * vPortDefineHeapRegions( xHeapRegions ); << Pass the array into vPortDefineHeapRegions().
 *
 * Note 0x80000000 is the lower address so appears in the array first.
 *
 */
#include "sdkconfig.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Defining MPU_WRAPPERS_INCLUDED_FROM_API_FILE prevents task.h from redefining
all the API functions to use the MPU wrappers.  That should only be done when
task.h is included from an application file. */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp8266/eagle_soc.h"
#include "rom/ets_sys.h"
#include "esp8266/rom_functions.h"

#ifdef MEMLEAK_DEBUG
#include "spi_flash.h"
extern esp_spi_flash_chip_t flashchip;
#endif

#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#define mtCOVERAGE_TEST_MARKER()

#if 1
#define mem_printf(fmt, args...) ets_printf(fmt,## args)
#else 
#define mem_printf(fmt, args...)
#endif

/* Block sizes must not get too small. */
#define heapMINIMUM_BLOCK_SIZE	( ( size_t ) ( uxHeapStructSize << 1 ) )

/* Assumes 8bit bytes! */
#define heapBITS_PER_BYTE		( ( size_t ) 8 )

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
} BlockLink_t;

/*-----------------------------------------------------------*/

/*
 * Inserts a block of memory that is being freed into the correct position in
 * the list of free memory blocks.  The block being freed will be merged with
 * the block in front it and/or the block behind it if the memory blocks are
 * adjacent to each other.
 */
static void prvInsertBlockIntoFreeList( BlockLink_t *pxBlockToInsert );

/*-----------------------------------------------------------*/

/* The size of the structure placed at the beginning of each allocated memory
block must by correctly byte aligned. */
static const uint32_t uxHeapStructSize	= ( ( sizeof ( BlockLink_t ) + ( portBYTE_ALIGNMENT - 1 ) ) & ~portBYTE_ALIGNMENT_MASK );

/* Create a couple of list links to mark the start and end of the list. */
static BlockLink_t xStart, *pxEnd = NULL;
#ifdef MEMLEAK_DEBUG
//add by jjj, we Link the used blocks here
static BlockLink_t yStart;
static size_t yFreeBytesRemaining;
#endif
/* Keeps track of the number of free bytes remaining, but says nothing about
fragmentation. */
static size_t xFreeBytesRemaining = 0;
static size_t xMinimumEverFreeBytesRemaining = 0;

/* Gets set to the top bit of an size_t type.  When this bit in the xBlockSize
member of an BlockLink_t structure is set then the block belongs to the
application.  When the bit is free the block is still part of the free heap
space. */
static size_t xBlockAllocatedBit = 0;

extern char _heap_start;
extern char _lit4_end;

static HeapRegion_t xHeapRegions[] =
{
    { NULL, 0 },
    { NULL, 0 },
    { NULL, 0 }
};

#ifdef MEMLEAK_DEBUG
#define pvPortMalloc_t(xWantedSize) pvPortMalloc_trace(xWantedSize, file, line, false)
#define vPortFree_t(pv) vPortFree_trace(pv, file, line)
#define pvPortCalloc_t(count, size) pvPortCalloc_trace(count, size, file, line)
#else
#define pvPortMalloc_t(xWantedSize) pvPortMalloc(xWantedSize)
#define vPortFree_t(pv) vPortFree(pv)
#define pvPortCalloc_t(count, size) pvPortCalloc(count, size)
#endif

/*-----------------------------------------------------------*/
bool __attribute__((weak)) check_memleak_debug_enable(void)
{
    return 0;
}

#ifdef MEMLEAK_DEBUG 
void prvInsertBlockIntoUsedList(BlockLink_t *pxBlockToInsert)
{
        BlockLink_t *pxIterator;
        for( pxIterator = &yStart; pxIterator->pxNextFreeBlock < pxBlockToInsert && pxIterator->pxNextFreeBlock != NULL;pxIterator = pxIterator->pxNextFreeBlock)
        {
                /* Nothing to do here. */
        }
        pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock;
        pxIterator->pxNextFreeBlock = pxBlockToInsert;
        yFreeBytesRemaining += pxBlockToInsert->xBlockSize;
}

#define SPI_FLASH_START ((void *)0x40100000)
#define SPI_FLASH_SIZE 0x100000
#define SPI_FLASH_END (SPI_FLASH_START + SPI_FLASH_SIZE)

void pvShowMalloc()
{
	BlockLink_t *pxIterator;
//ets_printf("sh0:%d,%d,",uxHeapStructSize,sizeof( BlockLink_t ));
	if(uxHeapStructSize < sizeof( BlockLink_t ))
		return;
	ETS_INTR_LOCK();
	Wait_SPI_Idle(&flashchip);
	Cache_Read_Enable_New();
//ets_printf("sh1,");
	printf("--------Show Malloc--------\n");
	for( pxIterator = &yStart; pxIterator->pxNextFreeBlock != NULL;pxIterator = pxIterator->pxNextFreeBlock) {
        BlockLink_t *blk = pxIterator->pxNextFreeBlock;
//ets_printf("sh2,");
		const char *basename = blk->file;
		/* This matches a similar check that vGetFileName() used to
		 * do, but the code seems fine without it
		if (SPI_FLASH_START <= (void *)basename && (void *)basename < SPI_FLASH_END) {
			basename = NULL;
		}
		*/
		/* The file contains the absolute path, try to shorten in by
		 * looking for path separators. Checks for both UNIX and
		 * Windows separators. */
		if (basename) {
			basename = strrchr(basename, '/');
			if (basename) {
				basename++;
			} else {
				basename = strrchr(blk->file, '\\');
				if (basename) {
					basename++;
				}
			}
		} else {
			basename = "";
		}
		if (((unsigned)-1) == blk->line)
			printf("F(1):%-30p       malloc %10d @ %p\n", blk->file, blk->xBlockSize - 0x80000000, ( void * ) ( ( ( unsigned char * ) blk ) + uxHeapStructSize));
		else
			printf("F(2):%-30sL:%4u malloc %10d @ %p\n", basename, blk->line, blk->xBlockSize - 0x80000000, ( void * ) ( ( ( unsigned char * ) blk ) + uxHeapStructSize));
//ets_printf("sh3,");
//		ets_delay_us(2000);
        system_soft_wdt_feed();
	}
	printf("--------Free %d--------\n\n", xFreeBytesRemaining);

#if 0
	uint32_t last_link = (uint32_t)yStart.pxNextFreeBlock;
	uint32_t index = 0;
	printf("'*':used, '-'free, each %d bytes\n", portBYTE_ALIGNMENT_v);
	printf("%x:", last_link);
	for( pxIterator = &yStart; pxIterator->pxNextFreeBlock != NULL;pxIterator = pxIterator->pxNextFreeBlock) {
	    uint16_t i;
	    for (i = 0; i < ((uint32_t)pxIterator->pxNextFreeBlock - last_link) / portBYTE_ALIGNMENT_v; i++) {
	        index++;
	        printf("-");
	        if (index % 64 == 0) {
	            printf("\n%x:", (uint32_t)yStart.pxNextFreeBlock + index * portBYTE_ALIGNMENT_v);
	        }
	    }
	    for (i = 0; i < pxIterator->pxNextFreeBlock->xBlockSize / portBYTE_ALIGNMENT_v; i++) {
	        index++;
	        printf("*");
	        if (index % 64 == 0) {
	            printf("\n%x:", (uint32_t)yStart.pxNextFreeBlock + index * portBYTE_ALIGNMENT_v);
            }
	    }
	    last_link = ((uint32_t)pxIterator->pxNextFreeBlock + pxIterator->pxNextFreeBlock->xBlockSize);
        system_soft_wdt_feed();
    }
	printf("\n\n");
#endif

//ets_printf("sh4\n");
	ETS_INTR_UNLOCK();
}

void system_show_malloc(void) __attribute__((alias("pvShowMalloc")));

int prvRemoveBlockFromUsedList(BlockLink_t *pxBlockToRemove)
{
        BlockLink_t *pxIterator;
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

	return xWantedSize;
}

/*
 * @brief add trace information to allocated memory
 */
void esp_mem_trace(const void *ptr, const char *trace, int no)
{
	BlockLink_t *pxLink = (BlockLink_t *)((uint8_t *)ptr - uxHeapStructSize);

	pxLink->file = trace;
	pxLink->line = (unsigned)no;
}

void *pvPortMalloc( size_t xWantedSize )
#ifdef MEMLEAK_DEBUG
{
	void *return_addr = (void *)__builtin_return_address(0);

    return pvPortMalloc_trace( xWantedSize, return_addr, (unsigned)-1, false );
}

void *pvPortMalloc_trace( size_t xWantedSize, const char * file, unsigned line, bool use_iram )
#endif
{
BlockLink_t *pxBlock, *pxPreviousBlock, *pxNewBlockLink;
void *pvReturn = NULL;
static bool is_inited = false;

    if (!is_inited) {
        void vPortDefineHeapRegions( const HeapRegion_t * const pxHeapRegions );
        xHeapRegions[0].pucStartAddress = ( uint8_t * )&_heap_start;
        xHeapRegions[0].xSizeInBytes = (( size_t)( 0x40000000 - (uint32_t)&_heap_start));
        
#ifndef CONFIG_SOC_FULL_ICACHE
        xHeapRegions[1].pucStartAddress = ( uint8_t * )&_lit4_end;
        xHeapRegions[1].xSizeInBytes = (( size_t)( 0x4010C000 - (uint32_t)&_lit4_end));
#endif

        is_inited = true;
        vPortDefineHeapRegions(xHeapRegions);
    }

	/* The heap must be initialised before the first call to
	prvPortMalloc(). */
	configASSERT( pxEnd );

//	vTaskSuspendAll();
	ETS_INTR_LOCK();
	{
		/* Check the requested block size is not so large that the top bit is
		set.  The top bit of the block size member of the BlockLink_t structure
		is used to determine who owns the block - the application or the
		kernel, so it must be free. */
		if( ( xWantedSize & xBlockAllocatedBit ) == 0 )
		{
			/* The wanted size is increased so it can contain a BlockLink_t
			structure in addition to the requested amount of bytes. */
			if( xWantedSize > 0 )
			{
				xWantedSize += uxHeapStructSize;

				/* Ensure that blocks are always aligned to the required number
				of bytes. */
				if( ( xWantedSize & portBYTE_ALIGNMENT_MASK ) != 0x00 )
				{
					/* Byte alignment required. */
					xWantedSize += ( portBYTE_ALIGNMENT - ( xWantedSize & portBYTE_ALIGNMENT_MASK ) );
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}

			if( ( xWantedSize > 0 ) && ( xWantedSize <= xFreeBytesRemaining ) )
			{
				/* Traverse the list from the start	(lowest address) block until
				one	of adequate size is found. */
				pxPreviousBlock = &xStart;
				pxBlock = xStart.pxNextFreeBlock;

                BlockLink_t *pxIterator;
                /* Iterate through the list until a block is found that has a higher address
                than the block being inserted. */
                for( pxIterator = &xStart; pxIterator->pxNextFreeBlock != 0; pxIterator = pxIterator->pxNextFreeBlock )
                {
                    if ((line == 0 || use_iram == true) && (uint32_t)pxIterator->pxNextFreeBlock > 0x40000000 && pxIterator->pxNextFreeBlock->xBlockSize > xWantedSize) {
                        pxPreviousBlock = pxIterator;
                        pxBlock = pxIterator->pxNextFreeBlock;
                        break;
                    }
                }

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
					BlockLink_t structure at its start. */
					pvReturn = ( void * ) ( ( ( uint8_t * ) pxPreviousBlock->pxNextFreeBlock ) + uxHeapStructSize );

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
						pxNewBlockLink = ( void * ) ( ( ( uint8_t * ) pxBlock ) + xWantedSize );

						/* Calculate the sizes of two blocks split from the
						single block. */
						pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
						pxBlock->xBlockSize = xWantedSize;

						/* Insert the new block into the list of free blocks. */
						prvInsertBlockIntoFreeList( ( pxNewBlockLink ) );
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}

					xFreeBytesRemaining -= pxBlock->xBlockSize;

					if( xFreeBytesRemaining < xMinimumEverFreeBytesRemaining )
					{
						xMinimumEverFreeBytesRemaining = xFreeBytesRemaining;
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}

					/* The block is being returned - it is allocated and owned
					by the application and has no "next" block. */
					pxBlock->xBlockSize |= xBlockAllocatedBit;
					pxBlock->pxNextFreeBlock = NULL;
                    
#ifdef MEMLEAK_DEBUG
					if(uxHeapStructSize >= sizeof( BlockLink_t )){
						pxBlock->file = file;
						pxBlock->line = line;
					}
					//link the use block
					prvInsertBlockIntoUsedList(pxBlock);
#endif
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}

		traceMALLOC( pvReturn, xWantedSize );
	}
	// ( void ) xTaskResumeAll();
    ETS_INTR_UNLOCK();

	#if( configUSE_MALLOC_FAILED_HOOK == 1 )
	{
		if( pvReturn == NULL )
		{
			extern void vApplicationMallocFailedHook( void );
			vApplicationMallocFailedHook();
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}
	#endif

	return pvReturn;
}

/*-----------------------------------------------------------*/

void vPortFree( void *pv )
#ifdef MEMLEAK_DEBUG
{
    void *return_addr = (void *)__builtin_return_address(0);

    vPortFree_trace(pv, return_addr, (unsigned)-1);
}

void vPortFree_trace( void *pv, const char * file, unsigned line )
#endif
{
uint8_t *puc = ( uint8_t * ) pv;
BlockLink_t *pxLink;

	if( pv != NULL )
	{
		/* The memory being freed will have an BlockLink_t structure immediately
		before it. */
		puc -= uxHeapStructSize;

		/* This casting is to keep the compiler from issuing warnings. */
		pxLink = ( void * ) puc;

		/* Check the block is actually allocated. */
		configASSERT( ( pxLink->xBlockSize & xBlockAllocatedBit ) != 0 );
		configASSERT( pxLink->pxNextFreeBlock == NULL );

		if( ( pxLink->xBlockSize & xBlockAllocatedBit ) != 0 )
		{
#ifndef MEMLEAK_DEBUG
			if( pxLink->pxNextFreeBlock == NULL )
#endif
			{
				/* The block is being returned to the heap - it is no longer
				allocated. */
				pxLink->xBlockSize &= ~xBlockAllocatedBit;

				//vTaskSuspendAll();
				ETS_INTR_LOCK();
#ifdef MEMLEAK_DEBUG
				if(prvRemoveBlockFromUsedList(pxLink) < 0){
					printf("%p already freed\n", pv);
				}
				else
#endif
				{
					/* Add this block to the list of free blocks. */
					xFreeBytesRemaining += pxLink->xBlockSize;
					traceFREE( pv, pxLink->xBlockSize );
					prvInsertBlockIntoFreeList( ( ( BlockLink_t * ) pxLink ) );
				}
				// ( void ) xTaskResumeAll();
                ETS_INTR_UNLOCK();
			}
#ifndef MEMLEAK_DEBUG
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
#endif
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}
}

/*-----------------------------------------------------------*/

void *pvPortCalloc(size_t count, size_t size)
#ifdef MEMLEAK_DEBUG
{
    void *return_addr = (void *)__builtin_return_address(0);

    return pvPortCalloc_trace(count, size, return_addr, (unsigned)-1);
}

void *pvPortCalloc_trace(size_t count, size_t size, const char * file, unsigned line)
#endif
{
    char *p = pvPortMalloc_trace(count * size, file, line, true);
    if (p)
        memset(p, 0, count * size);

    return p;
}

/*-----------------------------------------------------------*/

void *pvPortRealloc(void *mem, size_t newsize)
#ifdef MEMLEAK_DEBUG
{
	void *return_addr = (void *)__builtin_return_address(0);

    return pvPortRealloc_trace(mem, newsize, return_addr, (unsigned)-1);
}

void *pvPortRealloc_trace(void *mem, size_t newsize, const char *file, unsigned line)
#endif
{
    void *return_addr = (void *)__builtin_return_address(0);

    void *p = pvPortMalloc_trace(newsize, file, line, true);
    if (p && mem) {
        memcpy(p, mem, newsize);
        vPortFree_trace(mem, return_addr, 0);
    }

    return p;
}

/*-----------------------------------------------------------*/

void *pvPortZalloc(size_t size)
#ifdef MEMLEAK_DEBUG
{
    void *return_addr = (void *)__builtin_return_address(0);

    return pvPortZalloc_trace(size, return_addr, (unsigned)-1);
}
void *pvPortZalloc_trace(size_t size, const char * file, unsigned line)
#endif
{
    char *p = pvPortMalloc_trace(size, file, line, true);
    if (p)
        memset(p, 0, size);

    return p;
}

size_t xPortGetFreeHeapSize( void )
{
	return xFreeBytesRemaining;
}
/*-----------------------------------------------------------*/

size_t xPortGetMinimumEverFreeHeapSize( void )
{
	return xMinimumEverFreeBytesRemaining;
}
/*-----------------------------------------------------------*/

static void prvInsertBlockIntoFreeList( BlockLink_t *pxBlockToInsert )
{
BlockLink_t *pxIterator;
uint8_t *puc;

	/* Iterate through the list until a block is found that has a higher address
	than the block being inserted. */
	for( pxIterator = &xStart; pxIterator->pxNextFreeBlock < pxBlockToInsert; pxIterator = pxIterator->pxNextFreeBlock )
	{
		/* Nothing to do here, just iterate to the right position. */
	}

	/* Do the block being inserted, and the block it is being inserted after
	make a contiguous block of memory? */
	puc = ( uint8_t * ) pxIterator;
	if( ( puc + pxIterator->xBlockSize ) == ( uint8_t * ) pxBlockToInsert )
	{
		pxIterator->xBlockSize += pxBlockToInsert->xBlockSize;
		pxBlockToInsert = pxIterator;
	}
	else
	{
		mtCOVERAGE_TEST_MARKER();
	}

	/* Do the block being inserted, and the block it is being inserted before
	make a contiguous block of memory? */
	puc = ( uint8_t * ) pxBlockToInsert;
	if( ( puc + pxBlockToInsert->xBlockSize ) == ( uint8_t * ) pxIterator->pxNextFreeBlock )
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
	else
	{
		mtCOVERAGE_TEST_MARKER();
	}
}
/*-----------------------------------------------------------*/

void vPortDefineHeapRegions( const HeapRegion_t * const pxHeapRegions )
{
BlockLink_t *pxFirstFreeBlockInRegion = NULL, *pxPreviousFreeBlock;
uint8_t *pucAlignedHeap;
size_t xTotalRegionSize, xTotalHeapSize = 0;
uint8_t xDefinedRegions = 0;
uint32_t ulAddress;
const HeapRegion_t *pxHeapRegion;

	/* Can only call once! */
	configASSERT( pxEnd == NULL );

	pxHeapRegion = &( pxHeapRegions[ xDefinedRegions ] );
    
	while( pxHeapRegion->xSizeInBytes > 0 )
	{
		xTotalRegionSize = pxHeapRegion->xSizeInBytes;

		/* Ensure the heap region starts on a correctly aligned boundary. */
		ulAddress = ( uint32_t ) pxHeapRegion->pucStartAddress;
		if( ( ulAddress & portBYTE_ALIGNMENT_MASK ) != 0 )
		{
			ulAddress += ( portBYTE_ALIGNMENT - 1 );
			ulAddress &= ~portBYTE_ALIGNMENT_MASK;

			/* Adjust the size for the bytes lost to alignment. */
			xTotalRegionSize -= ulAddress - ( uint32_t ) pxHeapRegion->pucStartAddress;
		}

		pucAlignedHeap = ( uint8_t * ) ulAddress;

		/* Set xStart if it has not already been set. */
		if( xDefinedRegions == 0 )
		{
			/* xStart is used to hold a pointer to the first item in the list of
			free blocks.  The void cast is used to prevent compiler warnings. */
			xStart.pxNextFreeBlock = ( BlockLink_t * ) pucAlignedHeap;
			xStart.xBlockSize = ( size_t ) 0;
		}
		else
		{
			/* Should only get here if one region has already been added to the
			heap. */
			configASSERT( pxEnd != NULL );

			/* Check blocks are passed in with increasing start addresses. */
			configASSERT( ulAddress > ( uint32_t ) pxEnd );
		}

		/* Remember the location of the end marker in the previous region, if
		any. */
		pxPreviousFreeBlock = pxEnd;

		/* pxEnd is used to mark the end of the list of free blocks and is
		inserted at the end of the region space. */
		ulAddress = ( ( uint32_t ) pucAlignedHeap ) + xTotalRegionSize;
		ulAddress -= uxHeapStructSize;
		ulAddress &= ~portBYTE_ALIGNMENT_MASK;
		pxEnd = ( BlockLink_t * ) ulAddress;
		pxEnd->xBlockSize = 0;
		pxEnd->pxNextFreeBlock = NULL;

		/* To start with there is a single free block in this region that is
		sized to take up the entire heap region minus the space taken by the
		free block structure. */
		pxFirstFreeBlockInRegion = ( BlockLink_t * ) pucAlignedHeap;
		pxFirstFreeBlockInRegion->xBlockSize = ulAddress - ( uint32_t ) pxFirstFreeBlockInRegion;
		pxFirstFreeBlockInRegion->pxNextFreeBlock = pxEnd;

		/* If this is not the first region that makes up the entire heap space
		then link the previous region to this region. */
		if( pxPreviousFreeBlock != NULL )
		{
			pxPreviousFreeBlock->pxNextFreeBlock = pxFirstFreeBlockInRegion;
		}

		xTotalHeapSize += pxFirstFreeBlockInRegion->xBlockSize;

		/* Move onto the next HeapRegion_t structure. */
		xDefinedRegions++;
		pxHeapRegion = &( pxHeapRegions[ xDefinedRegions ] );
	}

	xMinimumEverFreeBytesRemaining = xTotalHeapSize;
	xFreeBytesRemaining = xTotalHeapSize;

	/* Check something was actually defined before it is accessed. */
	configASSERT( xTotalHeapSize );

	/* Work out the position of the top bit in a size_t variable. */
	xBlockAllocatedBit = ( ( size_t ) 1 ) << ( ( sizeof( size_t ) * heapBITS_PER_BYTE ) - 1 );
#ifdef MEMLEAK_DEBUG
	//add by jjj
    yStart.pxNextFreeBlock = NULL;
	yStart.xBlockSize = ( size_t ) 0;
    yFreeBytesRemaining = 0;
#endif
}
