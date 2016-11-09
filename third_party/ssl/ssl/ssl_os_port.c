/*
 * Copyright (c) 2007, Cameron Rich
 * 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, 
 *   this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice, 
 *   this list of conditions and the following disclaimer in the documentation 
 *   and/or other materials provided with the distribution.
 * * Neither the name of the axTLS project nor the names of its contributors 
 *   may be used to endorse or promote products derived from this software 
 *   without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file os_port.c
 *
 * OS specific functions.
 */

#include "ssl/ssl_os_port.h"
#include "lwip/sockets.h"

#ifdef MEMLEAK_DEBUG
static const char mem_debug_file[] ICACHE_RODATA_ATTR STORE_ATTR = __FILE__;
#endif

#ifdef WIN32
/**
 * gettimeofday() not in Win32 
 */
EXP_FUNC void STDCALL gettimeofday(struct timeval* t, void* timezone)
{       
#if defined(_WIN32_WCE)
    t->tv_sec = time(NULL);
    t->tv_usec = 0;                         /* 1sec precision only */ 
#else
    struct _timeb timebuffer;
    _ftime(&timebuffer);
    t->tv_sec = (long)timebuffer.time;
    t->tv_usec = 1000 * timebuffer.millitm; /* 1ms precision */
#endif
}

/**
 * strcasecmp() not in Win32
 */
EXP_FUNC int STDCALL strcasecmp(const char *s1, const char *s2)
{
    while (tolower(*s1) == tolower(*s2++))
    {
        if (*s1++ == '\0')
        {
            return 0;
        }
    }

    return *(unsigned char *)s1 - *(unsigned char *)(s2 - 1);
}


EXP_FUNC int STDCALL getdomainname(char *buf, int buf_size)
{
    HKEY hKey;
    unsigned long datatype;
    unsigned long bufferlength = buf_size;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
            TEXT("SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"),
                        0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
        return -1;

    RegQueryValueEx(hKey, "Domain", NULL, &datatype, buf, &bufferlength);
    RegCloseKey(hKey);
    return 0; 
}
#endif

static const char * out_of_mem_str = "out of memory %s %d\n";

#define exit_now	os_printf
//#define SSL_LOG
#ifdef	SSL_LOG
#define debug_now	os_printf
#else
#define debug_now
#endif

#if 0
static MEM_LEAK * ptr_start = NULL;  
static MEM_LEAK * ptr_next =  NULL;
xTaskHandle mem_mutex;
#define name_length 128

extern int mem_flag;
void add(MEM_INFO alloc_info)
{

    MEM_LEAK * mem_leak_info = NULL;
    mem_leak_info = (MEM_LEAK *)malloc(sizeof(MEM_LEAK));
    mem_leak_info->mem_info.address = alloc_info.address;
    mem_leak_info->mem_info.size = alloc_info.size;
    strcpy(mem_leak_info->mem_info.file_name, alloc_info.file_name); 
    mem_leak_info->mem_info.line = alloc_info.line;
    mem_leak_info->next = NULL;

    if (ptr_start == NULL)    
    {
        ptr_start = mem_leak_info;
        ptr_next = ptr_start;
    }
    else {
        ptr_next->next = mem_leak_info;
        ptr_next = ptr_next->next;                
    }
	if(mem_flag) {
		os_printf("mem_leak_info =%p\n",mem_leak_info);
		mem_flag = 0;
		report_mem_leak();
	}

}
void erase(unsigned pos)
{

    unsigned index = 0;
    MEM_LEAK * alloc_info, * temp;
    
    if(pos == 0)
    {
        MEM_LEAK * temp = ptr_start;
        ptr_start = ptr_start->next;
        free(temp);
    }
    else 
    {
        for(index = 0, alloc_info = ptr_start; index < pos; 
            alloc_info = alloc_info->next, ++index)
        {
            if(pos == index + 1)
            {
                temp = alloc_info->next;
                alloc_info->next =  temp->next;
                free(temp);
                break;
            }
        }
    }
}

void clear()
{
    MEM_LEAK * temp = ptr_start;
    MEM_LEAK * alloc_info = ptr_start;
	
    while(alloc_info != NULL) 
    {
        alloc_info = alloc_info->next;
        free(temp);
        temp = alloc_info;
    }
	
}

void add_mem_info (void * mem_ref, unsigned int size,  const char * file, unsigned int line)
{	  
	sys_mutex_lock(&mem_mutex);

	MEM_INFO mem_alloc_info;        
	memset( &mem_alloc_info, 0, sizeof ( mem_alloc_info ) );        
	mem_alloc_info.address     = mem_ref;        
	mem_alloc_info.size = size;        
	strncpy(mem_alloc_info.file_name, file, FILE_NAME_LENGTH);        
	mem_alloc_info.line = line;            
	add(mem_alloc_info);   
	sys_mutex_unlock(&mem_mutex);
}
void remove_mem_info (void * mem_ref)
{
    unsigned short index;
    MEM_LEAK  * leak_info = ptr_start;
	
	sys_mutex_lock(&mem_mutex);
    for(index = 0; leak_info != NULL; ++index, leak_info = leak_info->next)
    {
        if ( leak_info->mem_info.address == mem_ref )
        {
            erase ( index );
            break;
        }
    }
	
	sys_mutex_unlock(&mem_mutex);
}
#endif

EXP_FUNC void * ICACHE_FLASH_ATTR ax_malloc(size_t s, const char* file, int line)
{
    void *x;

    if ((x = malloc(s)) == NULL)
    	exit_now("out of memory %s %d\n", file, line);
    else {
    	debug_now("%s %d point[%p] size[%d] heap[%d]\n", file, line, x, s, system_get_free_heap_size());
		//add_mem_info(x, s, file, line);
    }

    return x;
}
EXP_FUNC void * ICACHE_FLASH_ATTR ax_realloc(void *y, size_t s, const char* file, int line)
{
    void *x;

    if ((x = realloc(y, s)) == NULL)
        exit_now("out of memory %s %d\n", file, line);
    else {
    	debug_now("%s %d point[%p] size[%d] heap[%d]\n", file, line, x, s, system_get_free_heap_size());	
		//add_mem_info(x, s, file, line);
    	}

    return x;
}
EXP_FUNC void * ICACHE_FLASH_ATTR ax_calloc(size_t n, size_t s, const char* file, int line)
{
    void *x;
	//unsigned total_size =0;
    if ((x = calloc(n, s)) == NULL)
    	exit_now("out of memory %s %d\n", file, line);
    else {
    	debug_now("%s %d point[%p] size[%d] heap[%d]\n", file, line, x, s, system_get_free_heap_size());
		//total_size = n * s;		 
		//add_mem_info (x, total_size, file, line);
    	}

    return x;
}
EXP_FUNC void * ICACHE_FLASH_ATTR ax_zalloc(size_t s, const char* file, int line)
{
    void *x;

    if ((x = (void*)zalloc(s)) == NULL)
    	exit_now("out of memory %s %d\n", file, line);
    else {
    	debug_now("%s %d point[%p] size[%d] heap[%d]\n", file, line, x, s, system_get_free_heap_size());
		//add_mem_info(x, s, file, line);
    	}

    return x;
}
EXP_FUNC void ICACHE_FLASH_ATTR ax_free(void *p, const char* file, int line)
{
	if(p) {
   		debug_now("%s %d point[%p] size[%d] heap[%d]\n", file, line, p,0, system_get_free_heap_size());
	   free(p);
	   p = NULL;
   }
   return ;
}

#if 0
void report_mem_leak(void)
{
    unsigned short index;
    MEM_LEAK * leak_info;

    char *info;
	sys_mutex_lock(&mem_mutex);
	os_printf("ptr_start =%p\n",ptr_start);
	info = (char *)zalloc(name_length);
	if(info) {
        for(leak_info = ptr_start; leak_info != NULL; leak_info = leak_info->next)
        {
			os_printf("%p\n",leak_info);
            sprintf(info, "address : %p\n", leak_info->mem_info.address);
            os_printf("%s\n",info);
            sprintf(info, "size    : %d bytes\n", leak_info->mem_info.size);            
            os_printf("%s\n",info);
            snprintf(info,name_length,"file    : %s\n", leak_info->mem_info.file_name);
            os_printf("%s\n",info);
            sprintf(info, "line    : %d\n", leak_info->mem_info.line);
            os_printf("%s\n",info);
        }
		clear();
		free(info);
	}
	sys_mutex_unlock(&mem_mutex);
	sys_mutex_free(&mem_mutex);
	
}


/*
void exit_now(const char *format, ...)
{
    va_list argp;

    va_start(argp, format);
    vfprintf(stderr, format, argp);
    va_end(argp);
    abort();
}*/
/**
 * gettimeofday() not in Win32 
 */
EXP_FUNC void STDCALL gettimeofday(struct timeval* t, void* timezone)
{       
#if defined(_WIN32_WCE)
    t->tv_sec = time(NULL);
    t->tv_usec = 0;                         /* 1sec precision only */ 
#else
    /* wujg : pass compile first */
    if (timezone != NULL)
    	t->tv_sec = *(time_t*)timezone + system_get_time()/1000000;
    else
    	t->tv_sec = system_get_time() + system_get_time()/1000000;
    t->tv_usec = 0; /* 1ms precision */
#endif
}
#endif

unsigned int def_private_key_len = 0;
unsigned char *def_private_key = NULL;
unsigned char *def_certificate = NULL;
unsigned int def_certificate_len = 0;

