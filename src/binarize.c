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


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <wchar.h>
#else
#include <errno.h>
#include <fts.h>
#endif

#include "args.h"
#include "filesystem.h"
#include "utils.h"
#include "rapify.h"
#include "binarize.h"
#include "utils.h"
#include "unistdwrapper.h"


#ifdef _WIN32


char *find_root(char *source) {
    char *root = (char *)malloc(2048);
    char *candidate = (char *)malloc(2048);

    wchar_t wc_source[2048];
    mbstowcs(wc_source, source, 2048);

    wchar_t wc_root[2048];
    mbstowcs(wc_root, root, 2048);


    GetFullPathName(wc_source, 2048, wc_root, NULL);
    wcstombs(root, wc_root, 2048);

    while (true) {
        if (strrchr(root, '\\') == NULL) {
            strcpy(candidate, "config.cpp");
            if (access(candidate, F_OK) != -1) {
                free(candidate);
                free(root);
                return ".\\";
            }
            return NULL;
        }

        *(strrchr(root, '\\') + 1) = 0;

        strcpy(candidate, root);
        strcat(candidate, "config.cpp");
        if (access(candidate, F_OK) != -1) {
            free(candidate);
            return root;
        }

        *(strrchr(root, '\\')) = 0;
    }
}

#endif


int binarize(char *source, char *target) {
    /*
     * Binarize the given file. If source and target are identical, the target
     * is overwritten. If the source is a P3D, it is converted to ODOL. If the
     * source is a rapifiable type (cpp, ext, etc.), it is rapified.
     *
     * If the file type is not recognized, -1 is returned. 0 is returned on
     * success and a positive integer on error.
     */

    char fileext[64];

    if (strchr(source, '.') == NULL)
        return -1;

    strncpy(fileext, strrchr(source, '.'), 64);

    if (!strcmp(fileext, ".cpp") ||
            !strcmp(fileext, ".rvmat") ||
            !strcmp(fileext, ".ext"))
        return rapify_file(source, target);

    return -1;
}


int cmd_binarize() {
    int success;

    if (args.num_positionals == 1) {
        return 128;
    } else if (args.num_positionals == 2) {
        success = binarize(args.positionals[1], "-");
    } else {
        // check if target already exists
        if (access(args.positionals[2], F_OK) != -1 && !args.force) {
            errorf("File %s already exists and --force was not set.\n", args.positionals[2]);
            return 1;
        }

        success = binarize(args.positionals[1], args.positionals[2]);
    }

    if (success == -1) {
        errorf("File is no P3D and doesn't seem rapifiable.\n");
        return 1;
    }

    return success;
}
