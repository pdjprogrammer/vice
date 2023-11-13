/** \file   rommanager.h
 * \brief   Settings dialog to manage ROMs - header
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
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
 */

#ifndef VICE_ROMMANAGER_H
#define VICE_ROMMANAGER_H

#include <gtk/gtk.h>

GtkWidget *rom_manager_new              (GtkWidget *parent);
void       rom_manager_add_machine_rom  (const char *label_text,
                                         const char *resource_name);
void       rom_manager_add_drive_rom    (const char *label_text,
                                         const char *resource_name);
void       rom_manager_add_drive_exp_rom(const char *label_text,
                                         const char *resource_name);

#endif
