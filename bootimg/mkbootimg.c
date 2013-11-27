/* tools/mkbootimg/mkbootimg.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "mincrypt/sha.h"
#include "bootimg.h"

static void *load_file(const char *fn, unsigned *_sz)
{
    char *data;
    int sz;
    int fd;

    data = 0;
    fd = open(fn, O_RDONLY);
    if(fd < 0) return 0;

    sz = lseek(fd, 0, SEEK_END);
    if(sz < 0) goto oops;

    if(lseek(fd, 0, SEEK_SET) != 0) goto oops;

    data = (char*) malloc(sz);
    if(data == 0) goto oops;

    if(read(fd, data, sz) != sz) goto oops;
    close(fd);

    if(_sz) *_sz = sz;
    return data;

oops:
    close(fd);
    if(data != 0) free(data);
    return 0;
}

int usage(void)
{
    fprintf(stderr,"usage: mkbootimg\n"
            "       --kernel <filename>\n"
            "       --ramdisk <filename>\n"
            "       [ --second <2ndbootloader-filename> ]\n"
            "       [ -t|--type [RTK|NORM] Type of boot img rockchip boot img has another addresess ]\n"
            "       [ --cmdline <kernel-commandline> ]\n"
            "       [ --board <boardname> ]\n"
            "       [ --base <address> ]\n"
            "       [ --pagesize <pagesize> ]\n"
            "       [ --ramdiskaddr <address> ]\n"
            "       -o|--output <filename>\n"
            );
    return 1;
}



static unsigned char padding[16384] = { 0, };

int write_padding(int fd, unsigned pagesize, unsigned itemsize)
{
    unsigned pagemask = pagesize - 1;
    unsigned count;

    if((itemsize & pagemask) == 0) {
        return 0;
    }

    count = pagesize - (itemsize & pagemask);

    if(write(fd, padding, count) != (signed)count) {
        return -1;
    } else {
        return 0;
    }
}

void parseType(const char *typeStr, enum boot_img_type *type, struct boot_img_consts *defs)
{
    if(!strcmp("rtk", typeStr) || !strcmp("RTK", typeStr)) {
        *defs = rtk_consts;
        *type = BOOT_IMG_TYPE_RTK;
    } else {
        *defs = norm_consts;
        *type = BOOT_IMG_TYPE_NORM;
    }
}

int main(int argc, char **argv)
{
    boot_img_hdr hdr;

    struct boot_img_consts defs = norm_consts;
    enum boot_img_type type;

    char *kernel_fn = 0;
    void *kernel_data = 0;
    char *ramdisk_fn = 0;
    void *ramdisk_data = 0;
    char *second_fn = 0;
    void *second_data = 0;
    char *cmdline = "";
    char *bootimg = 0;
    char *board = "";
    unsigned pagesize;
    unsigned base;
    int fd;
    SHA_CTX ctx;
    const uint8_t* sha;

    int pagesize_set_by_args = 0;
    int base_set_by_args = 0;
    int ramdiskaddr_set_by_args = 0;

    argc--;
    argv++;

    memset(&hdr, 0, sizeof(hdr));

    while(argc > 0){
        char *arg = argv[0];
        char *val = argv[1];
        if(argc < 2) {
            return usage();
        }
        argc -= 2;
        argv += 2;
        if(!strcmp(arg, "--output") || !strcmp(arg, "-o")) {
            bootimg = val;
        } else if(!strcmp(arg, "--kernel")) {
            kernel_fn = val;
        } else if(!strcmp(arg, "--ramdisk")) {
            ramdisk_fn = val;
        } else if(!strcmp(arg, "--second")) {
            second_fn = val;
        } else if(!strcmp(arg, "--cmdline")) {
            cmdline = val;
        } else if(!strcmp(arg, "--base")) {
            base = strtoul(val, 0, 16);
            base_set_by_args = 1;
        } else if(!strcmp(arg, "--ramdiskaddr")) {
            hdr.ramdisk_addr = strtoul(val, 0, 16);
            ramdiskaddr_set_by_args = 1;
        } else if(!strcmp(arg, "--board")) {
            board = val;
        } else if(!strcmp(arg,"--pagesize")) {
            pagesize = strtoul(val, 0, 10);
            if ((pagesize != 2048) && (pagesize != 4096) && (pagesize != 16384)) {
                fprintf(stderr,"error: unsupported page size %d\n", pagesize);
                return -1;
            }
            pagesize_set_by_args = 1;
        } else if(!strcmp(arg, "--type") || !strcmp(arg, "-t")) {
            parseType(val, &type, &defs);
        } else {
            return usage();
        }
    }
    if(!base_set_by_args)
        base = defs.base;
    if(!pagesize_set_by_args)
        pagesize = defs.pagesize;

    hdr.page_size    = pagesize;
    hdr.kernel_addr  = base + defs.kernel_addr_off;
    if(!ramdiskaddr_set_by_args)
        hdr.ramdisk_addr = base + defs.ramdisk_addr_off;
    hdr.second_addr  = base + defs.second_addr_off;
    hdr.tags_addr    = base + defs.tags_addr_off;


    if(bootimg == 0) {
        fprintf(stderr,"error: no output filename specified\n");
        return usage();
    }

    if(kernel_fn == 0) {
        fprintf(stderr,"error: no kernel image specified\n");
        return usage();
    }

    if(ramdisk_fn == 0) {
        fprintf(stderr,"error: no ramdisk image specified\n");
        return usage();
    }

    if(strlen(board) >= BOOT_NAME_SIZE) {
        fprintf(stderr,"error: board name too large\n");
        return usage();
    }

    strcpy((char *)hdr.name, board);

    memcpy(hdr.magic, BOOT_MAGIC, BOOT_MAGIC_SIZE);

    if(strlen(cmdline) > (BOOT_ARGS_SIZE - 1)) {
        fprintf(stderr,"error: kernel commandline too large\n");
        return 1;
    }
    strcpy((char*)hdr.cmdline, cmdline);

    kernel_data = load_file(kernel_fn, &hdr.kernel_size);
    if(kernel_data == 0) {
        fprintf(stderr,"error: could not load kernel '%s'\n", kernel_fn);
        return 1;
    }

    if(!strcmp(ramdisk_fn,"NONE")) {
        ramdisk_data = 0;
        hdr.ramdisk_size = 0;
    } else {
        ramdisk_data = load_file(ramdisk_fn, &hdr.ramdisk_size);
        if(ramdisk_data == 0) {
            fprintf(stderr,"error: could not load ramdisk '%s'\n", ramdisk_fn);
            return 1;
        }
    }

    if(second_fn) {
        second_data = load_file(second_fn, &hdr.second_size);
        if(second_data == 0) {
            fprintf(stderr,"error: could not load secondstage '%s'\n", second_fn);
            return 1;
        }
    }

    /* put a hash of the contents in the header so boot images can be
     * differentiated based on their first 2k.
     */
    SHA_init(&ctx);
    SHA_update(&ctx, kernel_data, (int)hdr.kernel_size);
    SHA_update(&ctx, &hdr.kernel_size, (int)sizeof(hdr.kernel_size));
    SHA_update(&ctx, ramdisk_data, hdr.ramdisk_size);
    SHA_update(&ctx, &hdr.ramdisk_size, (int)sizeof(hdr.ramdisk_size));
    SHA_update(&ctx, second_data, (int)hdr.second_size);
    SHA_update(&ctx, &hdr.second_size, (int)sizeof(hdr.second_size));
    if(type == BOOT_IMG_TYPE_RTK) {
        SHA_update(&ctx, &hdr.tags_addr, sizeof(hdr.tags_addr));
        SHA_update(&ctx, &hdr.page_size, sizeof(hdr.page_size));
        SHA_update(&ctx, &hdr.unused, sizeof(hdr.unused));
        SHA_update(&ctx, &hdr.name, sizeof(hdr.name));
        SHA_update(&ctx, &hdr.cmdline, sizeof(hdr.cmdline));
    }
    sha = SHA_final(&ctx);
    memcpy(hdr.id, sha,
           SHA_DIGEST_SIZE > sizeof(hdr.id) ? sizeof(hdr.id) : SHA_DIGEST_SIZE);

    fd = open(bootimg, O_CREAT | O_TRUNC | O_WRONLY, 0644);
    if(fd < 0) {
        fprintf(stderr,"error: could not create '%s'\n", bootimg);
        return 1;
    }

    if(write(fd, &hdr, sizeof(hdr)) != sizeof(hdr)) goto fail;
    if(write_padding(fd, pagesize, sizeof(hdr))) goto fail;

    if(write(fd, kernel_data, hdr.kernel_size) != (signed)hdr.kernel_size) goto fail;
    if(write_padding(fd, pagesize, hdr.kernel_size)) goto fail;

    if(write(fd, ramdisk_data, hdr.ramdisk_size) != (signed)hdr.ramdisk_size) goto fail;
    if(write_padding(fd, pagesize, hdr.ramdisk_size)) goto fail;

    if(second_data) {
        if(write(fd, second_data, hdr.second_size) != (signed)hdr.second_size) goto fail;
        if(write_padding(fd, pagesize, hdr.ramdisk_size)) goto fail;
    }

    return 0;

fail:
    unlink(bootimg);
    close(fd);
    fprintf(stderr,"error: failed writing '%s': %s\n", bootimg,
            strerror(errno));
    return 1;
}
