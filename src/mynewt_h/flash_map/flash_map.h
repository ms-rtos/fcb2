/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef FLASH_MAP_H
#define FLASH_MAP_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 *
 * Provides abstraction of flash regions for type of use.
 * I.e. dude where's my image?
 *
 * System will contain a map which contains flash areas. Every
 * region will contain flash identifier, offset within flash and length.
 *
 * 1. This system map could be in a file within filesystem (Initializer
 * must know/figure out where the filesystem is at).
 * 2. Map could be at fixed location for project (compiled to code)
 * 3. Map could be at specific place in flash (put in place at mfg time).
 *
 * Note that the map you use must be valid for BSP it's for,
 * match the linker scripts when platform executes from flash,
 * and match the target offset specified in download script.
 */
#include <stdbool.h>
#include <inttypes.h>

#include "syscfg/syscfg.h"

struct flash_area {
    uint8_t fa_id;
    uint8_t fa_device_id;
    uint16_t pad16;
    uint32_t fa_off;
    uint32_t fa_size;
#ifdef __MS_RTOS__
    int fd;
    ms_rawflash_geometry_t geometry;
#endif
};

struct flash_sector_range {
    struct flash_area fsr_flash_area;
    uint32_t fsr_range_start;
    uint16_t fsr_first_sector;
    uint16_t fsr_sector_count;
    uint32_t fsr_sector_size;
    uint8_t fsr_align;
};

/*
 * Start using flash area.
 */
int flash_area_open(uint8_t id, const struct flash_area **);

/*
 * End using flash area.
 */
int flash_area_close(const struct flash_area *);

/*
 * Read/write/erase. Offset is relative from beginning of flash area.
 */
int flash_area_read(const struct flash_area *, uint32_t off, void *dst,
  uint32_t len);
int flash_area_write(const struct flash_area *, uint32_t off, const void *src,
  uint32_t len);
int flash_area_erase(const struct flash_area *, uint32_t off, uint32_t len);

/*
 * Reads data. Return code indicates whether it thinks that
 * underlying area is in erased state.
 *
 * Returns 1 if empty, 0 if not. <0 in case of an error.
 */
int flash_area_read_is_empty(const struct flash_area *, uint32_t off, void *dst,
  uint32_t len);

/*
 * Given flash map area id, return info about sectors within the area.
 */
int flash_area_to_sector_ranges(int id, int *cnt,
  struct flash_sector_range *ret);

#ifdef __cplusplus
}
#endif

#endif /* FLASH_MAP_H */
