/*
 * Copyright (c) 2015-2020 ACOINFO Co., Ltd.
 * All rights reserved.
 *
 * Detailed license information can be found in the LICENSE file.
 *
 * File: ms_fcb2_porting.c Flash circular buffer porting.
 *
 * Author: Wang.Dongfang <wangdongfang@acoinfo.com>
 *
 */

#include <ms_rtos.h>

#include "syscfg/syscfg.h"
#include "os/os_mutex.h"
#include "flash_map/flash_map.h"

#include "ms_fcb2_cfg.h"

/**
 * @brief Flash circular buffer porting.
 */

void put_be16(void *buf, uint16_t x)
{
    uint8_t *u8ptr;

    u8ptr = buf;
    u8ptr[0] = (uint8_t)(x >> 8);
    u8ptr[1] = (uint8_t)x;
}

uint16_t get_be16(const void *buf)
{
    const uint8_t *u8ptr;
    uint16_t x;

    u8ptr = buf;
    x = (uint16_t)u8ptr[0] << 8;
    x |= u8ptr[1];

    return x;
}

/**
 * Create a mutex and initialize it.
 *
 * @param mu Pointer to mutex
 *
 * @return os_error_t
 *      OS_INVALID_PARM     Mutex passed in was NULL.
 *      OS_OK               no error.
 */
os_error_t os_mutex_init(struct os_mutex *mu)
{
    os_error_t err = OS_INVALID_PARM;

    if (mu != MS_NULL) {
        if (ms_mutex_create("fcb2_lock", MS_WAIT_TYPE_PRIO, &mu->id) == MS_ERR_NONE) {
            err = OS_OK;
        }
    }

    return err;
}

/**
 * Release a mutex.
 *
 * @param mu Pointer to the mutex to be released
 *
 * @return os_error_t
 *      OS_INVALID_PARM Mutex passed in was NULL.
 *      OS_BAD_MUTEX    Mutex was not granted to current task (not owner).
 *      OS_OK           No error
 */
os_error_t os_mutex_release(struct os_mutex *mu)
{
    os_error_t err = OS_INVALID_PARM;

    if (mu != MS_NULL) {
        if (ms_mutex_unlock(mu->id) == MS_ERR_NONE) {
            err = OS_OK;
        } else {
            err = OS_BAD_MUTEX;
        }
    }

    return err;
}

/**
 * Pend (wait) for a mutex.
 *
 * @param mu Pointer to mutex.
 * @param timeout Timeout, in os ticks.
 *                A timeout of 0 means do not wait if not available.
 *                A timeout of OS_TIMEOUT_NEVER means wait forever.
 *
 *
 * @return os_error_t
 *      OS_INVALID_PARM     Mutex passed in was NULL.
 *      OS_TIMEOUT          Mutex was owned by another task and timeout=0
 *      OS_OK               no error.
 */
os_error_t os_mutex_pend(struct os_mutex *mu, os_time_t timeout)
{
    os_error_t err = OS_INVALID_PARM;

    if (mu != MS_NULL) {
        if (ms_mutex_lock(mu->id, timeout) == MS_ERR_NONE) {
            err = OS_OK;
        } else {
            err = OS_TIMEOUT;
        }
    }

    return err;
}

/*
 * Start using flash area.
 */
int flash_area_open(uint8_t id, const struct flash_area **fap)
{
    int ret;
    struct flash_area *fa = ms_zalloc(sizeof(struct flash_area));

    *fap = NULL;

    if (fa != MS_NULL) {
        fa->fd = ms_io_open(MS_FCB2_RAWFLASH_PATH, O_WRONLY, 0666);
        if (fa->fd >= 0) {
            ret = ms_io_ioctl(fa->fd, MS_RAWFLASH_CMD_GET_GEOMETRY, &fa->geometry);
            if (ret == 0) {
                fa->fa_off = 0;
                fa->fa_size = fa->geometry.sector_size * fa->geometry.sector_size;
                *fap = fa;
            }
        } else {
            ret = -1;
        }
    } else {
        ret = -1;
    }

    return ret;
}

/*
 * End using flash area.
 */
int flash_area_close(const struct flash_area *fa)
{
    int ret;

    if (fa != MS_NULL) {
        ret = ms_io_close(fa->fd);
        ms_free((ms_ptr_t)fa);
    } else {
        ret = -1;
    }

    return ret;
}

/*
 * Read/write/erase. Offset is relative from beginning of flash area.
 */
int flash_area_read(const struct flash_area *fa, uint32_t off, void *dst, uint32_t len)
{
    int ret;

    if ((off >= fa->fa_size) || (off + len >= fa->fa_size)) {
        ret = -1;

    } else {
        ms_rawflash_msg_t msg;

        msg.memaddr = fa->fa_off + off;
        msg.buf = dst;
        msg.len = len;
        if (ms_io_read(fa->fd, &msg, sizeof(msg)) == sizeof(msg)) {
            ret = 0;
        } else {
            ret = -1;
        }
    }

    return ret;
}

int flash_area_write(const struct flash_area *fa, uint32_t off, const void *src, uint32_t len)
{
    int ret;

    if ((off >= fa->fa_size) || (off + len >= fa->fa_size)) {
        ret = -1;

    } else {
        ms_rawflash_msg_t msg;

        msg.memaddr = fa->fa_off + off;
        msg.buf = (ms_ptr_t)src;
        msg.len = len;
        if (ms_io_write(fa->fd, &msg, sizeof(msg)) == sizeof(msg)) {
            ret = 0;
        } else {
            ret = -1;
        }
    }

    return ret;
}

int flash_area_erase(const struct flash_area *fa, uint32_t off, uint32_t len)
{
    int ret;

    if ((off >= fa->fa_size) || (off + len >= fa->fa_size)) {
        ret = -1;

    } else {
        ms_rawflash_erase_t erase_msg;
        uint32_t start_sector;
        uint32_t start_addr;

        start_addr = fa->fa_off + off;
        start_sector = start_addr / fa->geometry.sector_size;
        start_addr = start_sector * fa->geometry.sector_size;

        len  += fa->fa_off + off - start_addr;
        len   = (len + fa->geometry.sector_size - 1) / fa->geometry.sector_size;

        erase_msg.sector = start_sector;
        erase_msg.count  = len;

        ret = ms_io_ioctl(fa->fd, MS_RAWFLASH_CMD_ERASE_SECTOR, &erase_msg);
    }

    return ret;
}

static int flash_is_erased(int fd, uint32_t addr, uint8_t *buf, size_t len)
{
    ms_rawflash_msg_t msg;
    int ret = 1; /* default erased */

    msg.memaddr = addr;
    msg.buf = buf;
    msg.len = len;
    if (ms_io_read(fd, &msg, sizeof(msg)) == sizeof(msg)) {
        int i;
        for (i = 0; i < len; i++) {
            if (buf[i] != 0xFF) {
                ret = 0; /* not erased */
                break;
            }
        }
    } else {
        ret = -1; /* error */
    }

    return ret;
}

/*
 * Reads data. Return code indicates whether it thinks that
 * underlying area is in erased state.
 *
 * Returns 1 if empty, 0 if not. <0 in case of an error.
 */
int flash_area_read_is_empty(const struct flash_area *fa, uint32_t off, void *dst, uint32_t len)
{
    return flash_is_erased(fa->fd, fa->fa_off + off, dst, len);
}

/*
 * Given flash map area id, return info about sectors within the area.
 */
int flash_area_to_sector_ranges(int id, int *cnt, struct flash_sector_range *fsr)
{
    int ret;

    *cnt = 1;

    if (fsr != MS_NULL) {
        struct flash_area *fa = &fsr->fsr_flash_area;

        bzero(fsr, sizeof(struct flash_sector_range));

        fa->fd = ms_io_open(MS_FCB2_RAWFLASH_PATH, O_WRONLY, 0666);
        if (fa->fd >= 0) {
            ret = ms_io_ioctl(fa->fd, MS_RAWFLASH_CMD_GET_GEOMETRY, &fa->geometry);
            if (ret == 0) {
                fa->fa_off = 0;
                fa->fa_size = fa->geometry.sector_size * fa->geometry.sector_size;

                fsr->fsr_sector_size = fa->geometry.sector_size;
                fsr->fsr_sector_count = fa->geometry.sector_count;
                fsr->fsr_align = 1;
            }
            ms_io_close(fa->fd);
            fa->fd = -1;

        } else {
            ret = -1;
        }
    } else {
        ret = 0;
    }

    return ret;
}
