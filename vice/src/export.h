/*
 * export.h - Expansion port and devices handling.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#ifndef VICE_EXPORT_H
#define VICE_EXPORT_H

#include "cartio.h"

typedef struct export_resource_s {
    const char *name;
    unsigned int game;
    unsigned int exrom; /* VIC20, CBM2, PLUS4: export "blocks" flags */
    io_source_t *io1;   /* VIC20: IO2 device, PLUS4: $fdxx device */
    io_source_t *io2;   /* VIC20: IO3 device, PLUS4: $fexx device */
    unsigned int cartid;
} export_resource_t;

typedef struct export_list_s {
    struct export_list_s *previous;
    export_resource_t *device;
    struct export_list_s *next;
} export_list_t;

/* returns head of list if param is NULL, else the next item */
export_list_t *export_query_list(export_list_t *item);
void export_dump(void);

int export_add(const export_resource_t *export_res);
int export_remove(const export_resource_t *export_res);

int export_resources_init(void);

#endif
