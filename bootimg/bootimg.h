/* tools/mkbootimg/bootimg.h
**
** Copyright 2007, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License"); 
** you may not use this file except in compliance with the License. 
** You may obtain a copy of the License at 
**
**     http://www.apache.org/licenses/LICENSE-2.0 
**
** Unless required by applicable law or agreed to in writing, software 
** distributed under the License is distributed on an "AS IS" BASIS, 
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
** See the License for the specific language governing permissions and 
** limitations under the License.
*/

#ifndef _BOOT_IMAGE_H_
#define _BOOT_IMAGE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct boot_img_hdr boot_img_hdr;

#define BOOT_MAGIC "ANDROID!"
#define BOOT_MAGIC_SIZE 8
#define BOOT_NAME_SIZE 16
#define BOOT_ARGS_SIZE 512


struct boot_img_consts
{
    unsigned kernel_addr_off;
    unsigned ramdisk_addr_off;
    unsigned second_addr_off;
    unsigned tags_addr_off;

    unsigned pagesize;
    unsigned base;
};

static const struct boot_img_consts norm_consts = {
    .kernel_addr_off  = 0x00008000,
    .ramdisk_addr_off = 0x01000000,
    .second_addr_off  = 0x00F00000,
    .tags_addr_off    = 0x00000100,

    .pagesize        = 2048,
    .base             = 0x10000000,
};

static const struct boot_img_consts rk_consts = {
    .kernel_addr_off  = 0x00408000,
    .ramdisk_addr_off = 0x02000000,
    .second_addr_off  = 0x00F00000,
    .tags_addr_off    = 0x00088000,

    .pagesize        = 16384,
    .base             = 0x60000000,
};

enum boot_img_type {
    BOOT_IMG_TYPE_NORM,
    BOOT_IMG_TYPE_RK,
};

struct boot_img_hdr
{
    unsigned char magic[BOOT_MAGIC_SIZE];

    unsigned kernel_size;  /* size in bytes */
    unsigned kernel_addr;  /* physical load addr */

    unsigned ramdisk_size; /* size in bytes */
    unsigned ramdisk_addr; /* physical load addr */

    unsigned second_size;  /* size in bytes */
    unsigned second_addr;  /* physical load addr */

    unsigned tags_addr;    /* physical addr for kernel tags */
    unsigned page_size;    /* flash page size we assume */
    unsigned unused[2];    /* future expansion: should be 0 */

    unsigned char name[BOOT_NAME_SIZE]; /* asciiz product name */
    
    unsigned char cmdline[BOOT_ARGS_SIZE];

    unsigned id[8]; /* timestamp / checksum / sha1 / etc */
};

/*
** +-----------------+ 
** | boot header     | 1 page
** +-----------------+
** | kernel          | n pages  
** +-----------------+
** | ramdisk         | m pages  
** +-----------------+
** | second stage    | o pages
** +-----------------+
**
** n = (kernel_size + page_size - 1) / page_size
** m = (ramdisk_size + page_size - 1) / page_size
** o = (second_size + page_size - 1) / page_size
**
** 0. all entities are page_size aligned in flash
** 1. kernel and ramdisk are required (size != 0)
** 2. second is optional (second_size == 0 -> no second)
** 3. load each element (kernel, ramdisk, second) at
**    the specified physical address (kernel_addr, etc)
** 4. prepare tags at tag_addr.  kernel_args[] is
**    appended to the kernel commandline in the tags.
** 5. r0 = 0, r1 = MACHINE_TYPE, r2 = tags_addr
** 6. if second_size != 0: jump to second_addr
**    else: jump to kernel_addr
*/

#if 0
typedef struct ptentry ptentry;

struct ptentry {
    char name[16];      /* asciiz partition name    */
    unsigned start;     /* starting block number    */
    unsigned length;    /* length in blocks         */
    unsigned flags;     /* set to zero              */
};

/* MSM Partition Table ATAG
**
** length: 2 + 7 * n
** atag:   0x4d534d70
**         <ptentry> x n
*/
#endif

//SOME GENERAL FUNCTIONS:

static void parseType(const char *typeStr, enum boot_img_type *type, struct boot_img_consts *defs)
{
    if(!strcmp("rk", typeStr) || !strcmp("RK", typeStr)) {
        *defs = rk_consts;
        *type = BOOT_IMG_TYPE_RK;
    } else if(!strcmp("norm", typeStr) || !strcmp("NORM", typeStr)){
        *defs = norm_consts;
        *type = BOOT_IMG_TYPE_NORM;
    } else {
		fprintf(stderr, "Unknown value of --type parameter.\n\n");
        exit(usage());
	}
}
#endif
