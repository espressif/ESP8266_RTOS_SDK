/*
 *  Copyright (c) 2010 - 2011 Espressif System
 *
 */

#ifndef __ESP_LIBC_H__
#define __ESP_LIBC_H__

char *strcpy(char *dst, const char *src);
char *strncpy(char *dst, const char *src, size_t n);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
size_t strlen(const char *s);
char *strstr(const char *s1, const char *s2);
char *strcat(char *dst, const char *src);
char *strncat(char *dst, const char *src, size_t count);
size_t strspn(const char *s, const char *accept);
size_t strcspn(const char *s, const char *reject);
char *strtok_r(char *s, const char *delim, char **ptrptr);
char *strtok(char *s, const char *delim);
char *strrchr(const char *s, int c);
char *strdup(const char *s);
char *strchr(const char *s, int c);
long strtol(const char *str, char **endptr, int base);

void bzero(void *s, size_t n);

void *memcpy(void *dst, const void *src, size_t n);
void *memset(void *dst, int c, size_t n);
int memcmp(const void *m1, const void *m2, size_t n);
void *memmove(void *dst, const void *src, size_t n);

int rand(void);

int printf(const char *format, ...);
int sprintf(char *out, const char *format, ...);
int snprintf(char *buf, unsigned int count, const char *format, ...);
int puts(const char *str);

void *malloc(size_t n);
void free(void *p);
void *calloc(size_t c, size_t n);
void *zalloc(size_t n);
void *realloc(void *p, size_t n);

int atoi(const char *s);
long atol(const char *s);

unsigned long os_random(void);
int os_get_random(unsigned char *buf, size_t len);

/* NOTE: don't use printf_opt in irq handler, for test */
#define os_printf(fmt, ...) do {	\
	static const char flash_str[] ICACHE_RODATA_ATTR STORE_ATTR = fmt;	\
	printf(flash_str, ##__VA_ARGS__);	\
	} while(0)

#endif /* __LIBC_H__ */
