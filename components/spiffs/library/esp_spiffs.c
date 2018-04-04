#include "esp_common.h"

#include <fcntl.h>
#include <stdio.h>

#define NUM_SYS_FD 3

static spiffs fs;

static u8_t *spiffs_work_buf;
static u8_t *spiffs_fd_buf;
static u8_t *spiffs_cache_buf;

#define FLASH_UNIT_SIZE 4

static s32_t esp_spiffs_readwrite(u32_t addr, u32_t size, u8_t *p, int write)
{
    /*
     * With proper configurarion spiffs never reads or writes more than
     * LOG_PAGE_SIZE
     */

    if (size > fs.cfg.log_page_size) {
        printf("Invalid size provided to read/write (%d)\n\r", (int) size);
        return SPIFFS_ERR_NOT_CONFIGURED;
    }

    char tmp_buf[fs.cfg.log_page_size + FLASH_UNIT_SIZE * 2];
    u32_t aligned_addr = addr & (-FLASH_UNIT_SIZE);
    u32_t aligned_size =
        ((size + (FLASH_UNIT_SIZE - 1)) & -FLASH_UNIT_SIZE) + FLASH_UNIT_SIZE;

    int res = spi_flash_read(aligned_addr, (u32_t *) tmp_buf, aligned_size);

    if (res != 0) {
        printf("spi_flash_read failed: %d (%d, %d)\n\r", res, (int) aligned_addr,
               (int) aligned_size);
        return res;
    }

    if (!write) {
        memcpy(p, tmp_buf + (addr - aligned_addr), size);
        return SPIFFS_OK;
    }

    memcpy(tmp_buf + (addr - aligned_addr), p, size);

    res = spi_flash_write(aligned_addr, (u32_t *) tmp_buf, aligned_size);

    if (res != 0) {
//	    printf("spi_flash_write failed: %d (%d, %d)\n\r", res,
//	              (int) aligned_addr, (int) aligned_size);
        return res;
    }

    return SPIFFS_OK;
}

static s32_t esp_spiffs_read(u32_t addr, u32_t size, u8_t *dst)
{
    return esp_spiffs_readwrite(addr, size, dst, 0);
}

static s32_t esp_spiffs_write(u32_t addr, u32_t size, u8_t *src)
{
    return esp_spiffs_readwrite(addr, size, src, 1);
}

static s32_t esp_spiffs_erase(u32_t addr, u32_t size)
{
    /*
     * With proper configurarion spiffs always
     * provides here sector address & sector size
     */
    if (size != fs.cfg.phys_erase_block || addr % fs.cfg.phys_erase_block != 0) {
        printf("Invalid size provided to esp_spiffs_erase (%d, %d)\n\r",
               (int) addr, (int) size);
        return SPIFFS_ERR_NOT_CONFIGURED;
    }

    return spi_flash_erase_sector(addr / fs.cfg.phys_erase_block);
}

s32_t esp_spiffs_init(struct esp_spiffs_config *config)
{
    if (SPIFFS_mounted(&fs)) {
        return -1;
    }

    spiffs_config cfg;
    s32_t ret;

    cfg.phys_size = config->phys_size;
    cfg.phys_addr = config->phys_addr;
    cfg.phys_erase_block = config->phys_erase_block;
    cfg.log_block_size = config->log_block_size;
    cfg.log_page_size = config->log_page_size;

    cfg.hal_read_f = esp_spiffs_read;
    cfg.hal_write_f = esp_spiffs_write;
    cfg.hal_erase_f = esp_spiffs_erase;

    if (spiffs_work_buf != NULL) {
        free(spiffs_work_buf);
        spiffs_work_buf = NULL;
    }
    spiffs_work_buf = malloc(config->log_page_size * 2);

    if (spiffs_work_buf == NULL) {
        return -1;
    }

    if (spiffs_fd_buf != NULL) {
        free(spiffs_fd_buf);
        spiffs_fd_buf = NULL;
    }
    spiffs_fd_buf = malloc(config->fd_buf_size);

    if (spiffs_fd_buf == NULL) {
        free(spiffs_work_buf);
        return -1;
    }

    if (spiffs_cache_buf != NULL) {
        free(spiffs_cache_buf);
        spiffs_cache_buf = NULL;
    }
    spiffs_cache_buf = malloc(config->cache_buf_size);

    if (spiffs_cache_buf == NULL) {
        free(spiffs_work_buf);
        free(spiffs_fd_buf);
        return -1;
    }

    ret =  SPIFFS_mount(&fs, &cfg, spiffs_work_buf,
                        spiffs_fd_buf, config->fd_buf_size,
                        spiffs_cache_buf, config->cache_buf_size,
                        0);

    if (ret == -1) {
        free(spiffs_work_buf);
        free(spiffs_fd_buf);
        free(spiffs_cache_buf);
    }

    return ret;        
}

void esp_spiffs_deinit(u8_t format)
{
    if (SPIFFS_mounted(&fs)) {
        SPIFFS_unmount(&fs);
        free(spiffs_work_buf);
        free(spiffs_fd_buf);
        free(spiffs_cache_buf);

        if (format) {
            SPIFFS_format(&fs);
        }
    }
}

int _open_r(struct _reent *r, const char *filename, int flags, int mode)
{
    spiffs_mode sm = 0;
    int res;
    int rw = (flags & 3);

    if (rw == O_RDONLY || rw == O_RDWR) {
        sm |= SPIFFS_RDONLY;
    }

    if (rw == O_WRONLY || rw == O_RDWR) {
        sm |= SPIFFS_WRONLY;
    }

    if (flags & O_CREAT) {
        sm |= SPIFFS_CREAT;
    }

    if (flags & O_TRUNC) {
        sm |= SPIFFS_TRUNC;
    }

    if (flags & O_APPEND) {
        sm |= SPIFFS_APPEND;
    }

    /* Supported in newer versions of SPIFFS. */
    /* if (flags && O_EXCL) sm |= SPIFFS_EXCL; */
    /* if (flags && O_DIRECT) sm |= SPIFFS_DIRECT; */

    res = SPIFFS_open(&fs, (char *) filename, sm, 0);

    if (res >= 0) {
        res += NUM_SYS_FD;
    }

    return res;
}

_ssize_t _read_r(struct _reent *r, int fd, void *buf, size_t len)
{

    ssize_t res;

    if (fd < NUM_SYS_FD) {
        res = -1;
    } else {
        res = SPIFFS_read(&fs, fd - NUM_SYS_FD, buf, len);
    }

    return res;
}

_ssize_t _write_r(struct _reent *r, int fd, void *buf, size_t len)
{

    if (fd < NUM_SYS_FD) {
        return -1;
    }

    int res = SPIFFS_write(&fs, fd - NUM_SYS_FD, (char *) buf, len);
    return res;
}

_off_t _lseek_r(struct _reent *r, int fd, _off_t where, int whence)
{

    ssize_t res;

    if (fd < NUM_SYS_FD) {
        res = -1;
    } else {
        res = SPIFFS_lseek(&fs, fd - NUM_SYS_FD, where, whence);
    }

    return res;
}

int _close_r(struct _reent *r, int fd)
{

    if (fd < NUM_SYS_FD) {
        return -1;
    }

    SPIFFS_close(&fs, fd - NUM_SYS_FD);
    return 0;
}

int _rename_r(struct _reent *r, const char *from, const char *to)
{

    int res = SPIFFS_rename(&fs, (char *) from, (char *) to);
    return res;
}

int _unlink_r(struct _reent *r, const char *filename)
{

    int res = SPIFFS_remove(&fs, (char *) filename);
    return res;
}

int _fstat_r(struct _reent *r, int fd, struct stat *s)
{

    int res;
    spiffs_stat ss;
    memset(s, 0, sizeof(*s));

    if (fd < NUM_SYS_FD) {
        s->st_ino = fd;
        s->st_rdev = fd;
        s->st_mode = S_IFCHR | 0666;
        return 0;
    }

    res = SPIFFS_fstat(&fs, fd - NUM_SYS_FD, &ss);

    if (res < 0) {
        return res;
    }

    s->st_ino = ss.obj_id;
    s->st_mode = 0666;
    s->st_nlink = 1;
    s->st_size = ss.size;
    return 0;
}
