/*
 * Copyright (C)  2016  Felix "KoffeinFlummi" Wiegand
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */


#ifndef _WIN32
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "minilzo.h"

#include "docopt.h"
#include "utils.h"
#include "paa2img.h"


int dxt12img(unsigned char *input, unsigned char *output, int width, int height) {
    /* Convert DXT1 data into a PNG image array. */

    int i;
    int j;

    typedef struct dxt1block {
        uint16_t c0 : 16;
        uint16_t c1 : 16;
        uint8_t cl3 : 2;
        uint8_t cl2 : 2;
        uint8_t cl1 : 2;
        uint8_t cl0 : 2;
        uint8_t cl7 : 2;
        uint8_t cl6 : 2;
        uint8_t cl5 : 2;
        uint8_t cl4 : 2;
        uint8_t cl11 : 2;
        uint8_t cl10 : 2;
        uint8_t cl9 : 2;
        uint8_t cl8 : 2;
        uint8_t cl15 : 2;
        uint8_t cl14 : 2;
        uint8_t cl13 : 2;
        uint8_t cl12 : 2;
    } dxt1block_t;

    for (i = 0; i < (width * height) / 2; i += 8) {
        dxt1block_t block;
        memcpy(&block, input + i, 8);

        uint8_t c[4][3] = {
            {
                255 * ((63488 & block.c0) >> 11) / 31,
                255 * ((2016 & block.c0) >> 5) / 63,
                255 * (31 & block.c0) / 31
            },
            {
                255 * ((63488 & block.c1) >> 11) / 31,
                255 * ((2016 & block.c1) >> 5) / 63,
                255 * (31 & block.c1) / 31
            }
        };
        c[2][0] = (2 * c[0][0] + 1 * c[1][0]) / 3;
        c[2][1] = (2 * c[0][1] + 1 * c[1][1]) / 3;
        c[2][2] = (2 * c[0][2] + 1 * c[1][2]) / 3;
        c[3][0] = (1 * c[0][0] + 2 * c[1][0]) / 3;
        c[3][1] = (1 * c[0][1] + 2 * c[1][1]) / 3;
        c[3][2] = (1 * c[0][2] + 2 * c[1][2]) / 3;

        unsigned int clookup[16] = {
            block.cl0, block.cl1, block.cl2, block.cl3,
            block.cl4, block.cl5, block.cl6, block.cl7,
            block.cl8, block.cl9, block.cl10, block.cl11,
            block.cl12, block.cl13, block.cl14, block.cl15
        };

        unsigned int x, y, index;
        for (j = 0; j < 16; j++) {
            x = ((i / 8) % (width / 4)) * 4 + 3 - (j % 4);
            y = ((i / 8) / (height / 4)) * 4 + (j / 4);
            index = (y * width + x) * 4;
            char pixel[4] = {
                c[clookup[j]][0],
                c[clookup[j]][1],
                c[clookup[j]][2],
                255
            };
            memcpy(output + index, pixel, 4);
        }
    }

    return 0;
}


int dxt52img(unsigned char *input, unsigned char *output, int width, int height) {
    /* Convert DXT5 data into a PNG image array. */

    int i;
    int j;

    /* For some reason, directly unpacking the alpha lookup table into the 16
     * 3-bit arrays didn't work, so i'm reading it into one 64bit integer and
     * unpacking it manually later. @todo */
    typedef struct dxt5block {
        uint8_t a0 : 8;
        uint8_t a1 : 8;
        uint64_t al : 48;
        uint16_t c0 : 16;
        uint16_t c1 : 16;
        uint8_t cl3 : 2;
        uint8_t cl2 : 2;
        uint8_t cl1 : 2;
        uint8_t cl0 : 2;
        uint8_t cl7 : 2;
        uint8_t cl6 : 2;
        uint8_t cl5 : 2;
        uint8_t cl4 : 2;
        uint8_t cl11 : 2;
        uint8_t cl10 : 2;
        uint8_t cl9 : 2;
        uint8_t cl8 : 2;
        uint8_t cl15 : 2;
        uint8_t cl14 : 2;
        uint8_t cl13 : 2;
        uint8_t cl12 : 2;
    } dxt5block_t;

    for (i = 0; i < width * height; i += 16) {
        dxt5block_t block;
        memcpy(&block, input + i, 16);

        uint8_t a[8] = {block.a0, block.a1};
        if (block.a0 > block.a1) { // @todo, this shouldn't be necessary
            a[2] = (6 * block.a0 + 1 * block.a1) / 7;
            a[3] = (5 * block.a0 + 2 * block.a1) / 7;
            a[4] = (4 * block.a0 + 3 * block.a1) / 7;
            a[5] = (3 * block.a0 + 4 * block.a1) / 7;
            a[6] = (2 * block.a0 + 5 * block.a1) / 7;
            a[7] = (1 * block.a0 + 6 * block.a1) / 7;
        } else {
            a[2] = (4 * block.a0 + 1 * block.a1) / 5;
            a[3] = (3 * block.a0 + 2 * block.a1) / 5;
            a[4] = (2 * block.a0 + 3 * block.a1) / 5;
            a[5] = (1 * block.a0 + 4 * block.a1) / 5;
            a[6] = 0;
            a[7] = 255;
        }

        // This is ugly, retarded and shouldn't be necessary. See above.
        unsigned int alookup[16] = {
            (block.al & 3584) >> 9,
            (block.al & 448) >> 6,
            (block.al & 56) >> 3,
            (block.al & 7) >> 0,
            (block.al & 14680064) >> 21,
            (block.al & 1835008) >> 18,
            (block.al & 229376) >> 15,
            (block.al & 28672) >> 12,
            (block.al & 60129542144) >> 33,
            (block.al & 7516192768) >> 30,
            (block.al & 939524096) >> 27,
            (block.al & 117440512) >> 24,
            (block.al & 246290604621824) >> 45,
            (block.al & 30786325577728) >> 42,
            (block.al & 3848290697216) >> 39,
            (block.al & 481036337152) >> 36
        };

        uint8_t c[4][3] = {
            {
                255 * ((63488 & block.c0) >> 11) / 31,
                255 * ((2016 & block.c0) >> 5) / 63,
                255 * (31 & block.c0) / 31
            },
            {
                255 * ((63488 & block.c1) >> 11) / 31,
                255 * ((2016 & block.c1) >> 5) / 63,
                255 * (31 & block.c1) / 31
            }
        };
        c[2][0] = (2 * c[0][0] + 1 * c[1][0]) / 3;
        c[2][1] = (2 * c[0][1] + 1 * c[1][1]) / 3;
        c[2][2] = (2 * c[0][2] + 1 * c[1][2]) / 3;
        c[3][0] = (1 * c[0][0] + 2 * c[1][0]) / 3;
        c[3][1] = (1 * c[0][1] + 2 * c[1][1]) / 3;
        c[3][2] = (1 * c[0][2] + 2 * c[1][2]) / 3;

        unsigned int clookup[16] = {
            block.cl0, block.cl1, block.cl2, block.cl3,
            block.cl4, block.cl5, block.cl6, block.cl7,
            block.cl8, block.cl9, block.cl10, block.cl11,
            block.cl12, block.cl13, block.cl14, block.cl15
        };

        unsigned int x, y, index;
        for (j = 0; j < 16; j++) {
            x = ((i / 16) % (width / 4)) * 4 + 3 - (j % 4);
            y = ((i / 16) / (height / 4)) * 4 + (j / 4);
            index = (y * width + x) * 4;
            char pixel[4] = {
                c[clookup[j]][0],
                c[clookup[j]][1],
                c[clookup[j]][2],
                a[alookup[j]]
            };
            memcpy(output + index, pixel, 4);
        }
    }

    return 0;
}


int paa2img(char *source, char *target) {
    /*
     * Converts PAA to PNG.
     *
     * Returns 0 on success and a positive integer on failure.
     */

    FILE *f;
    char taggsig[4];
    char taggname[4];
    unsigned char *compresseddata;
    unsigned char *imgdata;
    unsigned char *outputdata;
    uint32_t tagglen;
    uint32_t mipmap;
    uint32_t datalen;
    uint16_t paatype;
    uint16_t width;
    uint16_t height;
    int compression;
    int imgdatalen;
    lzo_uint out_len;

    f = fopen(args.source, "rb");
    if (!f) {
        errorf("Couldn't open source file.\n");
        return 1;
    }

    fread(&paatype, 2, 1, f);

    while (true) {
        fread(taggsig, 4, 1, f);
        taggsig[4] = 0x00;
        if (strcmp(taggsig, "GGAT")) {
            errorf("Failed to find MIPMAP pointer.\n");
            return 2;
        }

        fread(taggname, 4, 1, f);
        taggname[4] = 0x00;
        fread(&tagglen, 4, 1, f);
        if (strcmp(taggname, "SFFO")) {
            fseek(f, tagglen, SEEK_CUR);
            continue;
        }

        fread(&mipmap, 4, 1, f);
        break;
    }

    fseek(f, mipmap, SEEK_SET);
    fread(&width, sizeof(width), 1, f);
    fread(&height, sizeof(height), 1, f);
    fread(&datalen, 3, 1, f);

    compresseddata = (unsigned char *)malloc(datalen);
    fread(compresseddata, datalen, 1, f);
    fclose(f);

    compression = COMP_NONE;
    if (width % 32768 != width && (paatype == DXT1 || paatype == DXT3 || paatype == DXT5)) {
        width -= 32768;
        compression = COMP_LZO;
    } else if (paatype == ARGB4444 || paatype == ARGB1555 || paatype == AI88) {
        compression = COMP_LZSS;
    }

    imgdatalen = width * height;
    if (paatype == DXT1)
        imgdatalen /= 2;
    imgdata = malloc(imgdatalen);

    if (compression == COMP_LZO) {
        if (lzo_init() != LZO_E_OK) {
            errorf("Failed to init LZO decompressor - this shouldn't be happening ...\n");
            return 3;
        }
        out_len = imgdatalen;
        if (lzo1x_decompress(compresseddata, datalen, imgdata, &out_len, NULL) != LZO_E_OK) {
            errorf("Failed to decompress LZO data.\n");
            return 3;
        }
    } else if (compression == COMP_LZSS) {
        errorf("LZSS compression support is not implemented.\n");
        return 3;
    } else {
        memcpy(imgdata, compresseddata, imgdatalen);
    }

    outputdata = malloc(width * height * 4);

    switch (paatype) {
        case DXT1:
            if (dxt12img(imgdata, outputdata, width, height)) {
                errorf("DXT1 decoding failed.\n");
                return 4;
            }
            break;
        case DXT3:
            errorf("DXT3 support is not implemented.\n");
            return 4;
        case DXT5:
            if (dxt52img(imgdata, outputdata, width, height)) {
                errorf("DXT5 decoding failed.\n");
                return 4;
            }
            break;
        case ARGB4444:
            errorf("ARGB4444 support is not implemented.\n");
            return 4;
        case ARGB1555:
            errorf("ARGB1555 support is not implemented.\n");
            return 4;
        case AI88:
            errorf("GRAY / AI88 support is not implemented.\n");
            return 4;
        default:
            errorf("Unrecognized PAA type.\n");
            return 4;
    }

    free(imgdata);

    if (!stbi_write_png(args.target, width, height, 4, outputdata, width * 4)) {
        errorf("Failed to write image to output.\n");
        return 5;
    }

    free(outputdata);

    return 0;
}


int cmd_paa2img() {
    extern DocoptArgs args;

    // check if target already exists
    if (strcmp(args.target, "-") != 0 && access(args.target, F_OK) != -1 && !args.force) {
        errorf("File %s already exists and --force was not set.\n", args.target);
        return 1;
    }

    return paa2img(args.source, args.target);
}