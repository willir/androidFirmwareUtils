#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <libgen.h>

#include "mincrypt/sha.h"
#include "bootimg.h"

typedef unsigned char byte;

int read_padding(FILE* f, unsigned itemsize, int pagesize)
{
    byte* buf = (byte*)malloc(sizeof(byte) * pagesize);
    unsigned pagemask = pagesize - 1;
    unsigned count;

    if((itemsize & pagemask) == 0) {
        free(buf);
        return 0;
    }

    count = pagesize - (itemsize & pagemask);

    fread(buf, count, 1, f);
    free(buf);
    return count;
}

void write_string_to_file(char* file, char* string)
{
    FILE* f = fopen(file, "w");
    fwrite(string, strlen(string), 1, f);
    fwrite("\n", 1, 1, f);
    fclose(f);
}

int usage() {
    printf("usage: unpackbootimg\n");
    printf("\t-i|--input boot.img\n");
    printf("\t[ -o|--output output_directory]\n");
    printf("\t[ -p|--pagesize <size-in-hexadecimal> ]\n");
    printf("\t[ -t|--type [rtk|norm] Unpack rockChip boot.img (It's has another addresses offset...) ]\n");
    return 0;
}

void parseType(const char *type_str, enum boot_img_type *type, struct boot_img_consts *defs)
{
    if(!strcmp("rtk", type_str) || !strcmp("RTK", type_str)) {
        *defs = rtk_consts;
        *type = BOOT_IMG_TYPE_RTK;
    } else {
        *defs = norm_consts;
        *type = BOOT_IMG_TYPE_NORM;
    }
}

int main(int argc, char** argv)
{
    char tmp[PATH_MAX];
    char* directory = "./";
    char* filename = NULL;
    int pagesize = 0;

    enum boot_img_type type;
    struct boot_img_consts defs = norm_consts;

    argc--;
    argv++;
    printf("BUILDER:Andrew Huang <bluedrum@163.com>  http://bluedrum.sinaapp.com \n");
    while(argc > 0){
        char *arg = argv[0];
        char *val = argv[1];
        argc -= 2;
        argv += 2;
        if(!strcmp(arg, "--input") || !strcmp(arg, "-i")) {
            filename = val;
        } else if(!strcmp(arg, "--output") || !strcmp(arg, "-o")) {
            directory = val;
        } else if(!strcmp(arg, "--pagesize") || !strcmp(arg, "-p")) {
            pagesize = strtoul(val, 0, 16);
        } else if(!strcmp(arg, "--type") || !strcmp(arg, "-t")) {
            parseType(val, &type, &defs);
        } else {
            return usage();
        }
    }

    if (filename == NULL) {
        return usage();
    }

    int total_read = 0;
    FILE* f = fopen(filename, "rb");
    boot_img_hdr header;
    char boardName[BOOT_NAME_SIZE + 1] = {0};

    //printf("Reading header...\n");
    fread(&header, sizeof(header), 1, f);

    memcpy(boardName, header.name, BOOT_NAME_SIZE);

    printf("BOARD_KERNEL_CMDLINE %s\n", header.cmdline);
    printf("BOARD_KERNEL_BASE %08x\n", header.kernel_addr - defs.kernel_addr_off);
    printf("BOARD_PAGE_SIZE %d\n", header.page_size);
    printf("BOARD_NAME %s\n", boardName);

    if (pagesize == 0) {
        pagesize = header.page_size;
    }

    //printf("cmdline...\n");
    sprintf(tmp, "%s/%s", directory, basename(filename));
    strcat(tmp, "-cmdline");
    write_string_to_file(tmp, header.cmdline);
    
    //printf("base...\n");
    sprintf(tmp, "%s/%s", directory, basename(filename));
    strcat(tmp, "-base");
    char basetmp[200];
    sprintf(basetmp, "%08x", header.kernel_addr - defs.kernel_addr_off);
    write_string_to_file(tmp, basetmp);

    //printf("pagesize...\n");
    sprintf(tmp, "%s/%s", directory, basename(filename));
    strcat(tmp, "-pagesize");
    char pagesizetmp[200];
    sprintf(pagesizetmp, "%d", header.page_size);
    write_string_to_file(tmp, pagesizetmp);
    
    //printf("board name...\n");
    sprintf(tmp, "%s/%s-board", directory, basename(filename));
    write_string_to_file(tmp, boardName);
    
    total_read += sizeof(header);
    //printf("total read: %d\n", total_read);
    total_read += read_padding(f, sizeof(header), pagesize);

    sprintf(tmp, "%s/%s", directory, basename(filename));
    strcat(tmp, "-zImage");
    FILE *k = fopen(tmp, "wb");
    byte* kernel = (byte*)malloc(header.kernel_size);
    //printf("Reading kernel...\n");
    fread(kernel, header.kernel_size, 1, f);
    total_read += header.kernel_size;
    fwrite(kernel, header.kernel_size, 1, k);
    fclose(k);

    //printf("total read: %d\n", header.kernel_size);
    total_read += read_padding(f, header.kernel_size, pagesize);

    sprintf(tmp, "%s/%s", directory, basename(filename));
    strcat(tmp, "-ramdisk.gz");
    FILE *r = fopen(tmp, "wb");
    byte* ramdisk = (byte*)malloc(header.ramdisk_size);
    //printf("Reading ramdisk...\n");
    fread(ramdisk, header.ramdisk_size, 1, f);
    total_read += header.ramdisk_size;
    fwrite(ramdisk, header.ramdisk_size, 1, r);
    fclose(r);
    
    fclose(f);
    
    //printf("Total Read: %d\n", total_read);
    return 0;
}
